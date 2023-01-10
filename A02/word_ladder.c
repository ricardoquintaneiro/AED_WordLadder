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
//      *) breadth_first_search
//   4) RECOMMENDED: list all words belonginh to a connected component
//      *) breadth_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadth_first_search
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
//      *) breadth_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// static configuration
//

#define _max_word_size_  32


//
// data structures
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
  if (node == NULL) {
      fprintf(stderr, "allocate_adjacency_node: out of memory\n");
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
  if (node == NULL) {
      fprintf(stderr, "allocate_hash_table_node: out of memory\n");
      exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}


//
// hash table stuff (done)
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

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;
  unsigned int i;

  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if (hash_table == NULL) {
    fprintf(stderr, "create_hash_table: out of memory\n");
    exit(1);
  }
  hash_table->hash_table_size = 1024;
  hash_table->number_of_edges = 0;
  hash_table->number_of_entries = 0;
  hash_table->heads = (hash_table_node_t **)malloc(hash_table->hash_table_size * sizeof(hash_table_node_t *));
  if (hash_table->heads == NULL) {
    fprintf(stderr, "create_hash_table: out of memory\n");
    exit(1);
  }

  for (i = 0u; i < hash_table->hash_table_size; i++)
    hash_table->heads[i] = NULL;

  printf("Hash table created (size: %d)\n", hash_table->hash_table_size);

  return hash_table;
}

static void hash_table_grow(hash_table_t *hash_table)
{
  unsigned int i, new_size, hash_value;
  hash_table_node_t *node, *next_node;
  hash_table_node_t **new_heads;

  new_size = ((1.5) * hash_table->hash_table_size) + 1;

  new_heads = (hash_table_node_t **)malloc(new_size * sizeof(hash_table_node_t *));
  if (new_heads == NULL) {
    fprintf(stderr, "hash_table_grow: out of memory\n");
    exit(1);
  }

  for (i = 0u; i < new_size; i++)
    new_heads[i] = NULL;

  for (i = 0u; i < hash_table->hash_table_size; i++)
    for (node = hash_table->heads[i]; node != NULL; node = next_node) {
      next_node = node->next;
      hash_value = crc32(node->word) % new_size;
      node->next = new_heads[hash_value];
      new_heads[hash_value] = node;
      node = next_node;
    }
  free(hash_table->heads);
  hash_table->hash_table_size = new_size;
  hash_table->heads = new_heads;

  printf("Hash table resized (new size: %d, entries: %d)\n", new_size, hash_table->number_of_entries);
}


static void hash_table_free(hash_table_t *hash_table)
{
  unsigned int i;
  hash_table_node_t *node, *next_node;
  adjacency_node_t *adj_node, *next_adj_node;

  for (i = 0; i < hash_table->hash_table_size; i++) {
    for (node = hash_table->heads[i]; node != NULL; node = next_node) {
      next_node = node->next;
      for (adj_node = node->head; adj_node != NULL; adj_node = next_adj_node) {
        next_adj_node = adj_node->next;
        free_adjacency_node(adj_node);
      }
      free_hash_table_node(node);
    }
  }

  free(hash_table->heads);
  free(hash_table);

  printf("Hash table destroyed\n");
}

static hash_table_node_t *find_word(hash_table_t *hash_table, const char *word, int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;

  for (node = hash_table->heads[i]; node != NULL; node = node->next)
    if (strcmp(node->word, word) == 0)
      return node;

  if (insert_if_not_found == 1) {
    node = allocate_hash_table_node();
    strcpy(node->word, word);
    node->next = hash_table->heads[i];
    node->head = NULL;
    node->visited = 0;
    node->previous = NULL;
    node->representative = node;
    node->number_of_vertices = 1;
    node->number_of_edges = 0;
    hash_table->heads[i] = node;
    hash_table->number_of_entries++;
    if (hash_table->number_of_entries == hash_table->hash_table_size * 2)
      hash_table_grow(hash_table);
  }
  else
    node = NULL;

  return node;
}

//
// add edges to the word ladder graph (done)
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative,*next_node;

  representative = node;
  while (representative != representative->representative)
    representative = representative->representative;

  next_node = node;
  while (next_node != representative) {
    node = next_node;
    next_node = node->representative;
    node->representative = representative;
  }
  return representative;
}

static void add_edge(hash_table_t *hash_table,hash_table_node_t *from,const char *word)
{
  hash_table_node_t *to,*from_representative,*to_representative;
  adjacency_node_t *link;

  to = find_word(hash_table,word,0);

  if (to != NULL) {
    link = allocate_adjacency_node();
    link->next = from->head;
    link->vertex = to;
    from->head = link;

    link = allocate_adjacency_node();
    link->next = to->head;
    link->vertex = from;
    to->head = link;

    hash_table->number_of_edges += 1;

    from_representative = find_representative(from);
    to_representative = find_representative(to);

    if (from_representative != to_representative) {
      if (from_representative->number_of_vertices < to_representative->number_of_vertices) {
        from_representative->representative = to_representative;
        to_representative->number_of_vertices += from_representative->number_of_vertices;
        to_representative->number_of_edges += from_representative->number_of_edges + 1;
      }
      else {
        to_representative->representative = from_representative;
        from_representative->number_of_vertices += to_representative->number_of_vertices;
        from_representative->number_of_edges += to_representative->number_of_edges + 1;
      }
    }
    else {
      from_representative->number_of_edges += 1;
    }
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
// breadth-first search (done)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//


static int breadth_first_search(int maximum_number_of_vertices,hash_table_node_t **list_of_vertices,hash_table_node_t *origin,hash_table_node_t *goal)
{
  int r, w = 1;
  if (find_representative(origin) != find_representative(goal))
    return -1;
  for (int i = 0; i < maximum_number_of_vertices; i++)
    list_of_vertices[i] = NULL;

  list_of_vertices[0] = origin;
  origin->visited = 1;
  origin->previous = NULL;

  hash_table_node_t *current_node;
  adjacency_node_t *neighbor;
  hash_table_node_t *neighbor_node;

  for (r = 0; r < w; r++) {
    current_node = list_of_vertices[r];
    neighbor = current_node->head;
    while (neighbor != NULL) {
      if (goal != origin && goal->visited == 1)
        break;
      neighbor_node = neighbor->vertex;
      if (neighbor_node->visited == 0) {
        list_of_vertices[w++] = neighbor_node;
        neighbor_node->visited = 1;
        neighbor_node->previous = current_node;
      }
      neighbor = neighbor->next;
    }
  }

  if (goal->visited == 1)
    return w;
  return -1;
}

//
// list all vertices belonging to a connected component (done)
//

static void list_connected_component(hash_table_t *hash_table,const char *word)
{
  hash_table_node_t *origin = find_word(hash_table, word, 0);
  if (origin == NULL) {
    printf("The word is not in the hash table.\n");
    return;
  }

  hash_table_node_t *representative = find_representative(origin);

  unsigned int i;
  int maximum_number_of_vertices = representative->number_of_vertices;
  hash_table_node_t **vertices = (hash_table_node_t **)malloc(maximum_number_of_vertices * sizeof(hash_table_node_t *));
  if (vertices == NULL) {
    fprintf(stderr, "list_connected_component: out of memory\n");
    exit(1);
  }
  for (i = 0u; i < maximum_number_of_vertices; i++)
    vertices[i] = NULL;

  int number_of_vertices = breadth_first_search(maximum_number_of_vertices, vertices, representative, representative);

  printf("\nVertices in connected component:\n");
  for (i = 0u; i < number_of_vertices; i++)
    printf("%s\n", vertices[i]->word);
  printf("\n");

  for (i = 0u; i < number_of_vertices; i++) {
    vertices[i]->visited = 0;
    vertices[i]->previous = NULL;
  }
  free(vertices);
}

//
// compute the diameter of a connected component (optional) (done)
//

static int largest_diameter;
static hash_table_node_t **largest_diameter_example;

static int connected_component_diameter(hash_table_node_t *node) {
  int maximum_number_of_vertices = node->number_of_vertices;
  if (maximum_number_of_vertices == 1) {
    return 0;
  }

  int i;
  hash_table_node_t **all_vertices = (hash_table_node_t **)malloc(maximum_number_of_vertices * sizeof(hash_table_node_t *));
  hash_table_node_t **visited_vertices = (hash_table_node_t **)malloc(maximum_number_of_vertices * sizeof(hash_table_node_t *));

  if (all_vertices == NULL || visited_vertices == NULL) {
    fprintf(stderr, "connected_component_diameter: out of memory\n");
    exit(1);
  }
  for (i = 0; i < maximum_number_of_vertices; i++) {
    all_vertices[i] = NULL;
    visited_vertices[i] = NULL;
  }

  breadth_first_search(maximum_number_of_vertices, all_vertices, node, node);
  if (maximum_number_of_vertices > 500) {
    printf("Dealing with %s, %d nodes\r", node->word, maximum_number_of_vertices);
    fflush(stdout);
  }
  for (i = 0; i < maximum_number_of_vertices; i++) {
    all_vertices[i]->visited = 0;
    all_vertices[i]->previous = NULL;
  }

  int j;
  int visited = 0;
  int diameter = 0;
  hash_table_node_t *diameter_x;
  hash_table_node_t *diameter_y;
  hash_table_node_t *temp;

  for (i = 0; i < maximum_number_of_vertices; i++) {
    visited = breadth_first_search(maximum_number_of_vertices, visited_vertices, all_vertices[i], all_vertices[i]);
    temp = visited_vertices[visited - 1];
    for (j = 0; temp != NULL; j++)
      temp = temp->previous;
    if (j - 1 > diameter) {
      diameter_x = all_vertices[i];
      diameter_y = visited_vertices[visited - 1];
      diameter = j - 1;
    }
    for (j = 0; j < visited; j++) {
      visited_vertices[j]->visited = 0;
      visited_vertices[j]->previous = NULL;
    }
  }

  hash_table_node_t *diameter_array[diameter + 1];

  breadth_first_search(maximum_number_of_vertices, visited_vertices, diameter_x, diameter_y);
  for (i = 0; diameter_y->previous != NULL; i++) {
    diameter_array[i] = diameter_y;
    diameter_y = diameter_y->previous;
  }
  diameter_array[i] = diameter_x;

  if (diameter > largest_diameter) {
    largest_diameter = diameter;
    if (largest_diameter_example != NULL)
      free(largest_diameter_example);
    largest_diameter_example = (hash_table_node_t **)malloc((diameter + 1) * sizeof(hash_table_node_t *));
    for (i = 0; i < diameter + 1; i++) {
      largest_diameter_example[i] = diameter_array[i];
    }
  }

  for (i = 0; i < maximum_number_of_vertices; i++) {
    all_vertices[i]->visited = 0;
    all_vertices[i]->previous = NULL;
  }
  if (maximum_number_of_vertices > 500) {
    printf("                                       \r");
    fflush(stdout);
  }
  free(all_vertices);
  free(visited_vertices);
  return diameter;
}

//
// find the shortest path from a given word to another given word (done)
//

static void path_finder(hash_table_t *hash_table,const char *from_word,const char *to_word)
{
  hash_table_node_t *origin, *goal;
  origin = find_word(hash_table, to_word, 0);
  goal = find_word(hash_table, from_word, 0);

  if (origin == NULL || goal == NULL) {
    printf("One of the words is not in the hash table.\n");
    return;
  }

  if (find_representative(origin) != find_representative(goal)) {
    printf("The two words are not in the same connected component.\n");
    return;
  }

  int i, number_of_vertices, maximum_number_of_vertices;
  hash_table_node_t **vertices;
  maximum_number_of_vertices = origin->representative->number_of_vertices;

  vertices = (hash_table_node_t **)malloc(maximum_number_of_vertices * sizeof(hash_table_node_t *));
  if (vertices == NULL) {
    fprintf(stderr, "path_finder: out of memory\n");
    exit(1);
  }
  for (i = 0; i < maximum_number_of_vertices; i++)
    vertices[i] = NULL;

  number_of_vertices = breadth_first_search(maximum_number_of_vertices, vertices, origin, goal);

  if (number_of_vertices == -1) {
    printf("There is no path between the two words.\n");
  }
  else {
    printf("\npath from %s to %s:\n", from_word, to_word);
    for (i = 0; goal != NULL; i++) {
      printf(" [%3d] %s\n", i, goal->word);
      goal = goal->previous;
    }
    printf("\n");
  }
  for (i = 0; i < number_of_vertices; i++) {
    vertices[i]->visited = 0;
    vertices[i]->previous = NULL;
  }

  free(vertices);
}


//
// some graph information (optional) (done)
//

static void graph_info(hash_table_t *hash_table)
{
  unsigned int i, j = 0;
  hash_table_node_t *node;
  int number_of_connected_components = 0;
  int max_number_of_vertices_in_connected_component = 0;
  int max_number_of_edges_in_connected_component = 0;
  int *number_of_vertices_per_entry = (int *)calloc(hash_table->hash_table_size, sizeof(int));
  hash_table_node_t **representatives = (hash_table_node_t **)malloc(hash_table->number_of_entries * sizeof(hash_table_node_t *));

  if (representatives == NULL) {
    fprintf(stderr, "graph_info: out of memory\n");
    exit(1);
  }
  for (i = 0u; i < hash_table->number_of_entries; i++)
    representatives[i] = NULL;
  for (i = 0u; i < hash_table->hash_table_size; i++) {
    for (node = hash_table->heads[i]; node != NULL; node = node->next) {
      number_of_vertices_per_entry[i]++;
      if (find_representative(node) == node) {
        number_of_connected_components++;
        representatives[j++] = node;
        if (node->number_of_vertices > max_number_of_vertices_in_connected_component)
          max_number_of_vertices_in_connected_component = node->number_of_vertices;
        if (node->number_of_edges > max_number_of_edges_in_connected_component)
          max_number_of_edges_in_connected_component = node->number_of_edges;
      }
    }
  }

  printf("%d vertices\n", hash_table->number_of_entries);
  printf("%d edges\n", hash_table->number_of_edges);
  printf("%d connected components\n", number_of_connected_components);
  printf("at most %d vertices in a connected component\n", max_number_of_vertices_in_connected_component);
  printf("at most %d edges in a connected component\n", max_number_of_edges_in_connected_component);

  printf("On average %f vertices per hash table entry\n",
         (float)hash_table->number_of_entries / (float)hash_table->hash_table_size);

  // Estatísticas
  FILE *fp = fopen("stats.txt", "w");
  for (i = 0u; i < hash_table->hash_table_size; i++) {
    fprintf(fp, "%d\n", number_of_vertices_per_entry[i]);
  }
  fclose(fp);
  //

  int number_of_vertices[number_of_connected_components];
  int diameters[number_of_connected_components];
  for (i = 0; representatives[i] != NULL; i++) {
    diameters[i] = connected_component_diameter(representatives[i]);
    number_of_vertices[i] = representatives[i]->number_of_vertices;
  }

  int *cc_per_diameters = (int *)calloc(largest_diameter + 1, sizeof(int));
  int *cc_per_vertices = (int *)calloc(max_number_of_vertices_in_connected_component + 1, sizeof(int));
  for (i = 0; i < number_of_connected_components; i++) {
    cc_per_diameters[diameters[i]]++;
    cc_per_vertices[number_of_vertices[i]]++;
  }

  printf("\nnumber of connected components with a diameter of\n");
  for (i = 0; i < largest_diameter + 1; i++) {
    if (cc_per_diameters[i] > 0) {
      printf("  %d; %d\n", i, cc_per_diameters[i]);
    }
  }

  printf("\nnumber of connected components with a number of vertices of\n");
  for (i = 0; i < max_number_of_vertices_in_connected_component + 1; i++) {
    if (cc_per_vertices[i] > 0) {
      printf("  %d; %d\n", i, cc_per_vertices[i]);
    }
  }

  printf("\nlargest word ladder:\n");

  for (i = 0; i < largest_diameter + 1; i++) {
    printf(" [%3d] %s\n", i, largest_diameter_example[i]->word);
  }
  printf("\n");
  
  free(number_of_vertices_per_entry);
  free(largest_diameter_example);
  free(representatives);
  free(cc_per_diameters);
  free(cc_per_vertices);
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
