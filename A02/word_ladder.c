//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (speed run)
//
// Place your student numbers and names here
//   N.Mec. 108122  Name: Alexandre Pedro Ribeiro
//   N.Mec. 110056  Name: Ricardo Manuel Quintaneiro Almeida
//
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create
//      *) hash_table_grow
//      *) hash_table_free
//      *) find_word
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) find_representative
//      *) add_edge
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadh_first_search
//   4) RECOMMENDED: list all words belonginh to a connected component
//      *) breadh_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadh_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadh_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//


//                    graph vis, dot para representar grafos

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// static configuration
//

#define _max_word_size_  32


//
// data structures (SUGGESTION --- you may do it in a different way)
//

typedef struct adjacency_node_s  adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s      hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;            // link to the next adjacency list node
  hash_table_node_t *vertex;         // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_];        // the word
  hash_table_node_t *next;           // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;            // head of the linked list of adjancency edges
  int visited;                       // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous;       // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;      // the size of the hash table array
  unsigned int number_of_entries;    // the number of entries in the hash table
  unsigned int number_of_edges;      // number of edges (for information purposes only)
  hash_table_node_t **heads;         // the heads of the linked lists
};


//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}


//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if(table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i,j;

    for(i = 0u;i < 256u;i++)
      for(table[i] = i,j = 0u;j < 8u;j++)
        if(table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while(*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc;
}

static hash_table_t *hash_table_create(void)                         // CHECKED
{
  hash_table_t *hash_table;
  unsigned int i;

  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if(hash_table == NULL)
  {
    fprintf(stderr,"create_hash_table: out of memory\n");
    exit(1);
  }
  hash_table->hash_table_size = 100;
  hash_table->number_of_edges = 0;
  hash_table->number_of_entries = 0;
  hash_table->heads = (hash_table_node_t **)malloc(hash_table->hash_table_size * sizeof(hash_table_node_t *));
  if (hash_table->heads == NULL)
  {
    fprintf(stderr, "create_hash_table: out of memory\n");
    exit(1);
  }

  for (i = 0u; i < hash_table->hash_table_size; i++)
    hash_table->heads[i] = NULL;

  printf("Hash table created (size: %d)\n", hash_table->hash_table_size);
  
  return hash_table;
}

static void hash_table_grow(hash_table_t *hash_table) // MUDAR O NOME DO HASH_VALUE?
{
  unsigned int i, new_size, hash_value;
  hash_table_node_t *node, *next_node;
  hash_table_node_t **new_heads;

  // Double the size of the hash table
  new_size = ((1.5) * hash_table->hash_table_size) + 1;

  // Allocate a new array of heads for the new hash table
  new_heads = (hash_table_node_t **)malloc(new_size * sizeof(hash_table_node_t *));
  if(new_heads == NULL)
  {
    fprintf(stderr,"hash_table_grow: out of memory\n");
    exit(1);
  }

  // Initialize all the linked lists in the new hash table to empty
  for(i = 0u; i < new_size; i++)
    new_heads[i] = NULL;

  // Rehash all the nodes in the old hash table
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next) // ou node = next_node?
    {
      next_node = node->next;
      // Compute the new hash value for this node
      hash_value = crc32(node->word) % new_size;
      // Add the node to the linked list of the corresponding bucket in the new hash table
      node->next = new_heads[hash_value];
      new_heads[hash_value] = node;
      node = next_node;
    }
  // Free the old array of heads and update the size and heads fields of the hash table structure
  free(hash_table->heads);
  hash_table->hash_table_size = new_size;
  hash_table->heads = new_heads;

  printf("Hash table resized (new size: %d, entries: %d)\n",new_size,hash_table->number_of_entries);
}


static void hash_table_free(hash_table_t *hash_table)
{
  unsigned int i;
  hash_table_node_t *node, *next_node;
  adjacency_node_t *adj_node, *next_adj_node;

  // Free all the nodes and adjacency lists in the hash table
  for (i = 0; i < hash_table->hash_table_size; i++)
  {
    for (node = hash_table->heads[i]; node != NULL; node = next_node)
    {
      next_node = node->next;
      // Free the adjacency list of this node
      for (adj_node = node->head; adj_node != NULL; adj_node = next_adj_node)
      {
        next_adj_node = adj_node->next;
        free_adjacency_node(adj_node);
      }
      // Free the node itself
      free_hash_table_node(node);
    }
  }

  // Free the array of heads and the hash table structure
  free(hash_table->heads);
  free(hash_table);
  
  printf("Hash table destroyed\n");
}

// static hash_table_node_t *find_word(hash_table_t *hash_table,const char *word,int insert_if_not_found)
// {
//   hash_table_node_t *node;
//   unsigned int i;

//   i = crc32(word) % hash_table->hash_table_size;
  
//   hash_table_node_t *list = hash_table->heads[i];
  
//   if (insert_if_not_found == 1) {
// 	hash_table_node_t *newItem = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
// 	*newItem->word = word;
// 	newItem->next = NULL;
//   }
  
  
//   if (list == NULL) {
// 	if (insert_if_not_found == 1) {
// 		hash_table->heads[i] = newItem;
// 		return newItem;
// 	}
// 	else {
// 		return list;
// 		// Aqui não sei bem o que é para dar return quando não encontra a palavra
// 	}
//   }
//   else {
// 	  while (list->next != NULL) {
// 			  list = list->next;
// 			  if (strcmp(list-> word,word) == 0) return list;
// 	  }
// 	  if (insert_if_not_found == 1) {
// 		  list->next = newItem;
// 		  return newItem;
// 	  }
// 	  return list;
//   }
//   //
//   // COMPLETED ?
//   //
//   return node;
// }

static hash_table_node_t *find_word(hash_table_t *hash_table, const char *word, int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;

  // Search for the word in the linked list of the corresponding bucket
  for (node = hash_table->heads[i]; node != NULL; node = node->next)
    if (strcmp(node->word, word) == 0)
      return node; // The word was found

  if (insert_if_not_found == 1) // Should the word be inserted if it was not found?
  {
    if (hash_table->number_of_entries == hash_table->hash_table_size * 2)
      hash_table_grow(hash_table);
    // Allocate a new node and insert it at the beginning of the linked list
    node = allocate_hash_table_node();
    strcpy(node->word, word);
    node->next = hash_table->heads[i];
    node->head = NULL;            // Initialize the adjacency list to empty
    node->visited = 0;            // Initialize the visited flag to 0
    node->previous = NULL;        // Initialize the parent pointer to NULL
    node->representative = node;  // Initialize the representative of the connected component to itself
    node->number_of_vertices = 1; // Initialize the number of vertices in the connected component to 1 (itself)
    node->number_of_edges = 0;    // Initialize the number of edges in the connected component to 0
    hash_table->heads[i] = node;
    hash_table->number_of_entries++;
  }
  else
    node = NULL; // The word was not found and should not be inserted

  return node;
}

//
// add edges to the word ladder graph (mostly do be done)
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative,*next_node;

  // A segunda vez que se chama esta função tem que ir logo ao representante
  // em vez de percorrer cada ponteiro até lá chegar

  representative = node;
  while(representative != representative->representative)
    representative = representative->representative;

  // Compress the path from node to the representative
  next_node = node;
  while(next_node != representative)
  {
    node = next_node;
    next_node = node->representative;
    node->representative = representative;
  }
  return representative;
}

// static hash_table_node_t *find_representative(hash_table_node_tnode)
// {
// // find the representative of the connected component this node belongs to
// if (node->representative != node)
// node->representative = find_representative(node->representative);
// return node->representative;
// }

static void add_edge(hash_table_t *hash_table,hash_table_node_t *from,const char *word)
{
  hash_table_node_t *to,*from_representative,*to_representative;
  adjacency_node_t *link;

  to = find_word(hash_table,word,0);

  // SE A PALAVRA NÃO ESTIVER NA HASH TABLE NÃO ACONTECE NADA
  // SE A PALAVRA EXISTIR, CRIAR 2 ARESTAS, UMA PARA CADA LADO?
  if (to != NULL)
  {
    // Allocate a new adjacency node
    link = allocate_adjacency_node();
    link->next = from->head;
    link->vertex = to;
    from->head = link;

    // Find the representatives of the connected components
    from_representative = find_representative(from);
    to_representative = find_representative(to);

    // Check if the vertices belong to different connected components
    if (from_representative != to_representative)
    {
      // Merge the two connected components
      if (from_representative->number_of_vertices < to_representative->number_of_vertices)
      {
        // The from_representative connected component has fewer vertices, so it becomes part of the to_representative connected component
        from_representative->representative = to_representative;
        to_representative->number_of_vertices += from_representative->number_of_vertices;
        to_representative->number_of_edges += from_representative->number_of_edges;
      }
      else
      {
        // The to_representative connected component has fewer vertices, so it becomes part of the from_representative connected component
        to_representative->representative = from_representative;
        from_representative->number_of_vertices += to_representative->number_of_vertices;
        from_representative->number_of_edges += to_representative->number_of_edges;
      }
    }

    //// ALTERNATIVA

    // from_representative = find_representative(from); // Find the representative of the connected component of the 'from' vertex
    // to_representative = find_representative(to);     // Find the representative of the connected component of the 'to' vertex

    // if (from_representative == to_representative) // The 'from' and 'to' vertices are already in the same connected component
    //   return;                                     // The edge cannot be added (it would create a cycle)

    // // Add the edge to the adjacency list of the 'from' vertex
    // link = allocate_adjacency_node();
    // link->vertex = to;
    // link->next = from->head;
    // from->head = link;

    // // Update the number of edges in the connected component of the 'from' vertex
    // from_representative->number_of_edges++;

    // // Merge the connected components of the 'from' and 'to' vertices
    // to_representative->representative = from_representative;
    // from_representative->number_of_vertices += to_representative->number_of_vertices;
    // from_representative->number_of_edges += to_representative->number_of_edges;

    // // Update the global count of edges in the graph
    // hash_table->number_of_edges++;
  }
}
//
// generates a list of similar words and calls the function add_edge for each one (done)
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word,int *individual_characters)
{
  int byte0,byte1;

  while(*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if(byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr,"break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters,char word[_max_word_size_])
{
  int code;

  while(*individual_characters != 0)
  {
    code = *(individual_characters++);
    if(code < 0x80)
      *(word++) = (char)code;
    else if(code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr,"make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0';  // mark the end
}

static void similar_words(hash_table_t *hash_table,hash_table_node_t *from)
{
  static const int valid_characters[] =
  { // unicode!
    0x2D,                                                                       // -
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,           // A B C D E F G H I J K L M
    0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,           // N O P Q R S T U V W X Y Z
    0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,           // a b c d e f g h i j k l m
    0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,           // n o p q r s t u v w x y z
    0xC1,0xC2,0xC9,0xCD,0xD3,0xDA,                                              // Á Â É Í Ó Ú
    0xE0,0xE1,0xE2,0xE3,0xE7,0xE8,0xE9,0xEA,0xED,0xEE,0xF3,0xF4,0xF5,0xFA,0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
    0
  };
  int i,j,k,individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word,individual_characters);
  for(i = 0;individual_characters[i] != 0;i++)
  {
    k = individual_characters[i];
    for(j = 0;valid_characters[j] != 0;j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters,new_word);
      // avoid duplicate cases
      if(strcmp(new_word,from->word) > 0)
        add_edge(hash_table,from,new_word);
    }
    individual_characters[i] = k;
  }
}


//
// breadth-first search (to be done)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//

typedef struct queue_node
{
  hash_table_node_t *value;
  struct queue_node *next;
} queue_node_t;

static int breadh_first_search(int maximum_number_of_vertices,hash_table_node_t **list_of_vertices,hash_table_node_t *origin,hash_table_node_t *goal)
{
  // USAR UMA FILA (QUEUE) COM UM ARRAY
  // para o tamanho usar o numero de vertices (do componente conexo)

  // Initialize the queue
  queue_node_t *head = NULL;
  queue_node_t *tail = NULL;

  // Enqueue the origin node
  queue_node_t *new_node = malloc(sizeof(queue_node_t));
  new_node->value = origin;
  new_node->next = NULL;

  if (head == NULL)
  {
    head = new_node;
    tail = new_node;
  }
  else
  {
    tail->next = new_node;
    tail = new_node;
  }

  int distance = 0;
  while (head != NULL)
  {
    // Dequeue the next node and check if it is the goal
    if (head->value == goal)
    {
      return distance;
    }

    head = head->next;
    distance++;

    // Enqueue the neighbors of the dequeued node
    for (int i = 0; i < maximum_number_of_vertices; i++)
    {
      if (list_of_vertices[i] != NULL && list_of_vertices[i]->visited == 0)
      {
        queue_node_t *new_node = malloc(sizeof(queue_node_t));
        new_node->value = list_of_vertices[i];
        new_node->next = NULL;

        if (head == NULL)
        {
          head = new_node;
          tail = new_node;
        }
        else
        {
          tail->next = new_node;
          tail = new_node;
        }

        list_of_vertices[i]->visited = 1;
      }
    }
  }

  // Return -1 if the goal is not found
  return -1;


  // Alternativa

  // int i, number_of_vertices_visited = 0;
  // hash_table_node_t *queue = malloc(maximum_number_of_vertices * sizeof(hash_table_node_t));
  // if (queue == NULL)
  // {
  //   fprintf(stderr, "breadth_first_search: out of memory\n");
  //   exit(1);
  // }

  // // Initialize the queue with the origin node
  // queue[0] = origin;
  // origin->visited = 1;
  // origin->previous = NULL;

  // // Perform the breadth-first search
  // for (i = 0; i < maximum_number_of_vertices; i++)
  // {
  //   hash_table_node_t *current_node = queue[i];
  //   if (current_node == goal)
  //   {
  //     // Goal was reached, return the number of vertices visited
  //     number_of_vertices_visited = i + 1;
  //     break;
  //   }
  // Add the unvisited neighbors of the current node to the queue
  // adjacency_node_t *neighbor = current_node->head;
  // while (neighbor != NULL)
  // {
  //   hash_table_node_t *neighbor_node = neighbor->node;
  //   if (neighbor_node->visited == 0)
  //   {
  //     queue[i + 1] = neighbor_node;
  //     neighbor_node->visited = 1;
  //     neighbor_node->previous = current_node;
  //   }
  //   neighbor = neighbor->next;
  // }
  // // Free the queue
  // free(queue);
  // return number_of_vertices_visited;
  // }
}

//
// list all vertices belonging to a connected component (complete this)
//

static void list_connected_component(hash_table_t *hash_table,const char *word)
{
  //
  // complete this
  //
}


//
// compute the diameter of a connected component (optional)
//

static int largest_diameter;
static hash_table_node_t **largest_diameter_example;

static int connected_component_diameter(hash_table_node_t *node)
{
  int diameter = 0;
  int number_of_vertices = node->number_of_vertices;
  hash_table_node_t *list_of_vertices = malloc(number_of_vertices * sizeof(hash_table_node_t));
  if (list_of_vertices == NULL)
  {
    fprintf(stderr, "connected_component_diameter: out of memory\n");
    exit(1);
  }

  // Create a list of all the vertices in the connected component
  hash_table_node_t *current_node = node;
  for (int i = 0; i < number_of_vertices; i++)
  {
    list_of_vertices[i] = *current_node;
    current_node = current_node->next;
  }

  // Find the diameter of the connected component by trying all pairs of vertices
  for (int i = 0; i < number_of_vertices; i++)
  {
    for (int j = 0; j < number_of_vertices; j++)
    {
      if (i != j)
      {
        int distance = breadh_first_search(number_of_vertices, list_of_vertices, &list_of_vertices[i], &list_of_vertices[j]);
        if (distance > diameter)
        {
          diameter = distance;
          largest_diameter = diameter;
          largest_diameter_example = list_of_vertices;
        }
      }
    }
  }

  free(list_of_vertices);
  return diameter;

  // Alternativa

  // int diameter, number_of_vertices, i;
  // hash_table_node_t **list_of_vertices;

  // // Allocate an array for the vertices of the connected component
  // number_of_vertices = node->number_of_vertices;
  // list_of_vertices = (hash_table_node_t **)malloc(number_of_vertices * sizeof(hash_table_node_t *));
  // if (list_of_vertices == NULL)
  // {
  //   fprintf(stderr, "connected_component_diameter: out of memory\n");
  //   exit(1);
  // }

  // // Fill the array with the vertices of the connected component
  // i = 0;
  // list_of_vertices[i++] = node;
  // while (i < number_of_vertices)
  // {
  //   node->visited = 1;
  //   adjacency_node_t *adj_node;
  //   for (adj_node = node->head; adj_node != NULL; adj_node = adj_node->next)
  //     if (adj_node->node->visited == 0)
  //       list_of_vertices[i++] = adj_node->node;
  //   node = list_of_vertices[i - 1];
  // }

  // // Compute the diameter of the connected component
  // diameter = 0;
  // for (i = 0; i < number_of_vertices; i++)
  // {
  //   node = list_of_vertices[i];
  //   node->visited = 0;
  //   int distance = breadh_first_search(number_of_vertices, list_of_vertices, node, node);
  //   if (distance > diameter)
  //     diameter = distance;
  // }

  // // Free the array of vertices
  // free(list_of_vertices);
  // return diameter;
}


//
// find the shortest path from a given word to another given word (to be done)
//

static void path_finder(hash_table_t *hash_table,const char *from_word,const char *to_word)
{
  hash_table_node_t *origin, *goal;
  origin = find_word(hash_table, from_word, 0);
  goal = find_word(hash_table, to_word, 0);

  if (origin == NULL || goal == NULL)
  {
    printf("One of the words is not in the hash table.\n");
    return;
  }

  if (find_representative(origin) != find_representative(goal))
  {
    printf("The two words are not in the same connected component.\n");
    return;
  }

  int number_of_vertices;
  hash_table_node_t **vertices;

  // Allocate an array to store the list of vertices in the connected component
  vertices = (hash_table_node_t **)malloc(connected_component_diameter(origin) * sizeof(hash_table_node_t *));
  if (vertices == NULL)
  {
    fprintf(stderr, "path_finder: out of memory\n");
    exit(1);
  }

  // Obtain the list of vertices in the connected component

  number_of_vertices = breadh_first_search(connected_component_diameter(origin), vertices, origin, goal);

  if (number_of_vertices == -1)
  {
    printf("There is no path between the two words.\n");
  }
  else
  {
    // Print the shortest path
    printf("Shortest path: %d\n", number_of_vertices - 1);
    while (goal != origin)
    {
      printf("%s\n", goal->word);
      goal = goal->previous;
    }
    printf("%s\n", origin->word);
  }

  free(vertices);
}


//
// some graph information (optional)
//

static void graph_info(hash_table_t *hash_table)
{
  // para listar o nº de componentes conexos ver o nº de representativos diferentes

  unsigned int i;
  int number_of_vertices = 0;
  int number_of_edges = 0;
  int number_of_connected_components = 0;
  int max_number_of_vertices_in_connected_component = 0;
  int max_number_of_edges_in_connected_component = 0;
  for (unsigned int i = 0; i < hash_table->hash_table_size; i++)
  {
    hash_table_node_t *node;
    for (node = hash_table->heads[i]; node != NULL; node = node->next)
    {
      number_of_vertices++;
      number_of_edges += node->number_of_edges;
      if (node->representative == node)
      {
        number_of_connected_components++;
        if (node->number_of_vertices > max_number_of_vertices_in_connected_component)
          max_number_of_vertices_in_connected_component = node->number_of_vertices;
        if (node->number_of_edges > max_number_of_edges_in_connected_component)
          max_number_of_edges_in_connected_component = node->number_of_edges;
      }
    }
  }
  printf("Number of vertices: %d\n", number_of_vertices);
  printf("Number of edges: %d\n", number_of_edges);
  printf("Number of connected components: %d\n", number_of_connected_components);
  printf("at most %d vertices in a connected component\n", max_number_of_vertices_in_connected_component);
  printf("at most %d edges in a connected component\n", max_number_of_edges_in_connected_component);

  // int i, num_vertices = 0, num_edges = 0, num_components = 0;
  // int max_vertices = 0, max_edges = 0, num_diameter_components = 0;
  // hash_table_node_t *node;

  // for (i = 0; i < hash_table->hash_table_size; i++)
  // {
  //   for (node = hash_table->heads[i]; node != NULL; node = node->next)
  //   {
  //     // Count the number of vertices and edges
  //     num_vertices++;
  //     num_edges += node->number_of_edges;

  //     // Update the maximum number of vertices and edges in a connected component
  //     if (node->number_of_vertices > max_vertices)
  //       max_vertices = node->number_of_vertices;
  //     if (node->number_of_edges > max_edges)
  //       max_edges = node->number_of_edges;

  //     // Check if this node is part of a connected component with the same diameter as the largest diameter found so far
  //     if (node->number_of_vertices == largest_diameter_example[0]->number_of_vertices)
  //       num_diameter_components++;
  //   }
  // }

  // // Print the number of vertices, edges, and connected components
  // printf("Number of vertices: %d\n", num_vertices);
  // printf("Number of edges: %d\n", num_edges);
  // printf("Number of connected components: %d\n", num_components);

  // // Print the maximum number of vertices and edges in a connected component
  // printf("Maximum number of vertices in a connected component: %d\n", max_vertices);
  // printf("Maximum number of edges in a connected component: %d\n", max_edges);

  // // Print the number of connected components with the same diameter as the largest diameter found so far
  // printf("Number of connected components with the same diameter as the largest: %d\n", num_diameter_components);

  // // Print the largest word ladder
  // printf("Largest word ladder:\n");
  // for (i = 0; i < largest_diameter; i++)
  //   printf("%s\n", largest_diameter_example[i]->word);

  // unsigned int i, num_vertices = 0, num_edges = 0, num_connected_components = 0;
  // int max_vertices = 0, max_edges = 0, diameter_count = 0;
  // hash_table_node_t *node;

  // // Iterate over all the nodes in the hash table
  // for (i = 0; i < hash_table->hash_table_size; i++)
  // {
  //   for (node = hash_table->heads[i]; node != NULL; node = node->next)
  //   {
  //     // If the node has not been visited yet, it is part of a new connected component
  //     if (!node->visited)
  //     {
  //       num_connected_components++;
  //       num_vertices++;
  //       // Breadth-first search to visit all the nodes in the connected component
  //       num_vertices += breadth_first_search(hash_table->number_of_entries, &node, node, NULL);
  //       // Update the maximum number of vertices in a connected component if necessary
  //       if (num_vertices > max_vertices)
  //         max_vertices = num_vertices;
  //       // Reset the number of vertices for the next connected component
  //       num_vertices = 0;
  //       // Update the maximum number of edges in a connected component if necessary
  //       if (node->number_of_edges > max_edges)
  //         max_edges = node->number_of_edges;
  //       // Update the count of connected components with the same diameter if necessary
  //       if (node->largest_diameter == largest_diameter)
  //         diameter_count++;
  //     }
  //     else
  //     {
  //       num_vertices++;
  //       num_edges += node->number_of_edges;
  //     }
  //   }

  // // Print the number of vertices, edges, and connected components
  // printf("Number of vertices: %u\n", num_vertices);
  // printf("Number of edges: %u\n", num_edges);
  // printf("Number of connected components: %u\n", num_connected_components);
  // // Print the maximum number of vertices and edges in a connected component
  // printf("Maximum number of vertices in a connected component: %d\n", max_vertices);
  // printf("Maximum number of edges in a connected component: %d\n", max_edges);
  // // Print the number of connected components with the same diameter as the largest one
  // printf("Number of connected components with the largest diameter: %d\n", diameter_count);
  // // Print the largest word ladder
  // printf("Largest word ladder: %s\n", largest_diameter_example[0]->word);
  // for (i = 1; i < largest_diameter; i++)
  //   printf(" -> %s", largest_diameter_example[i]->word);
  // printf("\n");
}


//
// main program
//

int main(int argc,char **argv)
{
  char word[100],from[100],to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;

  // initialize hash table
  hash_table = hash_table_create();
  // read words
  fp = fopen((argc < 2) ? "wordlist-big-latest.txt" : argv[1],"rb");
  if(fp == NULL)
  {
    fprintf(stderr,"main: unable to open the words file\n");
    exit(1);
  }
  while(fscanf(fp,"%99s",word) == 1)
    (void)find_word(hash_table,word,1);
  fclose(fp);
  // find all similar words
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next)
      similar_words(hash_table,node);
  graph_info(hash_table);
  // ask what to do
  for(;;)
  {
    fprintf(stderr,"Your wish is my command:\n");
    fprintf(stderr,"  1 WORD       (list the connected component WORD belongs to)\n");
    fprintf(stderr,"  2 FROM TO    (list the shortest path from FROM to TO)\n");
    fprintf(stderr,"  3            (terminate)\n");
    fprintf(stderr,"> ");
    if(scanf("%99s",word) != 1)
      break;
    command = atoi(word);
    if(command == 1)
    {
      if(scanf("%99s",word) != 1)
        break;
      list_connected_component(hash_table,word);
    }
    else if(command == 2)
    {
      if(scanf("%99s",from) != 1)
        break;
      if(scanf("%99s",to) != 1)
        break;
      path_finder(hash_table,from,to);
    }
    else if(command == 3)
      break;
  }
  // clean up
  hash_table_free(hash_table);
  return 0;
}
