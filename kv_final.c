#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Struct that describes the key-value pair
typedef struct KeyValue {
  char key[10];
  char val[30];
} KeyValue;


//Struct that describes a LinkedList node. Each node contains the KeyValue pair as the data (val) and pointer to the next node
typedef struct Node {
    struct KeyValue val;
    struct Node *next;
} Node;

//This method is used to push a new node to the head of a linked list. Refer to Readme diagram to see what it does
void push( Node **head, KeyValue val) {
    Node *node = malloc(sizeof(Node));

    node->val = val;
    node->next = *head;
    *head = node;
}


int main(int argc, char *argv[]) {
  int i;
  char *inp;
  char *inputs[argc][3];
  char value[30];
  FILE *fp;
  size_t len = 0;
  char *line = NULL;
  int itr;

  //Initialize the hashtable containing LinkedList node. 100 such nodes are present, each initially containing the KeyValue with key as "-1"
  KeyValue kv = {.key = "-1"};
  Node *hashtable[100] = {0};
  for(i = 0; i < 100; i++) {
    Node *currN = malloc(sizeof(Node));
    strcpy(currN->val.key, "-1");
    hashtable[i] = currN;
  }

  //Append existing file or create new file if doesn't exist. This will be the persistent store for holding all the key value pairs after the program
  //execution completes
  fp = fopen("key_value_new.csv", "a+");
  int currHash = -1;
  char currKey[10];

  //The while loop below loads the whole file in memory, into the hashtable above.
  while( getline(&line, &len, fp) > 1) {
      strcpy(currKey, line);
      char *token = strtok(currKey, ",");
      char *valToken = strtok(NULL, "\n");
      currHash = atoi(token) % 100;
      strcpy(kv.key, token);
      strcpy(kv.val, valToken);
      if(atoi(hashtable[currHash]->val.key) == -1) {
       push(&hashtable[currHash], kv);
      } else {
        Node *tempHead = malloc(sizeof(Node));
        tempHead = hashtable[currHash];
        
        while(tempHead != NULL
        && atoi(tempHead->val.key) != -1
        && strcmp(tempHead->val.key, token) != 0) {
            tempHead = tempHead->next;
        }
        push(&hashtable[currHash], kv);  
      }
    }
  fclose(fp);

  for(i = 1; i < argc; i++) {
    int j;
    for(j = 0; j < 3; j++) {
      inputs[i][j] = malloc(sizeof(char *));
      inputs[i][j] = NULL;
    }
  }

  //For each of the inputs, get the command called, and the arguments, and perform actions based on the command
  for(i = 1; i < argc; i++) {
    int k=0;
    while( (inp = strsep(&argv[i], ",")) != NULL) {
      if(k == 3) {
		printf("bad command");
		break;
	  }
      inputs[i][k] = inp;
      k++;
    }
  
    if(k > 3) {
     printf("bad command\n");
    }
    
    char command = (char) inputs[i][0][0];
    int hashVal;
    
    switch(command) {
	//When user invokes the 'put' command: 
        case 'p':
          if(inputs[i][1] == NULL || inputs[i][2] == NULL) {
		    printf("bad command\n");
		    break;
	      }
          int length = strlen(inputs[i][1]);
          for(itr = 0; itr < length; itr++) {
            if(!isdigit(inputs[i][1][itr])) {
              printf("bad command\n");
              break;
            }
          }
          hashVal = atoi(inputs[i][1]) % 100;
          strcpy(kv.key, inputs[i][1]);
          strcpy(kv.val, inputs[i][2]);
          
	  //Check if the array at index = hash of the input key contains any value other than -1.
	  //If no, add new Node with input as the KeyValue as the head, push Node with key "-1" to next.
          if( atoi(hashtable[hashVal]->val.key) == -1) {
            push(&hashtable[hashVal], kv);
          } 
	  //If yes, compare the key at the current node with the input key, and do so till a node with key as input key is found or end of linked list.
	  else {
            Node *tempHead = malloc(sizeof(Node));
            tempHead = hashtable[hashVal];
            while(tempHead != NULL 
             && atoi(tempHead->val.key) != -1
             && strcmp(tempHead->val.key, inputs[i][1]) != 0) {
              tempHead = tempHead->next;
            }
	    //If both are the same, then update the value at the current node.
            if(tempHead != NULL 
             && strcmp(tempHead->val.key, inputs[i][1]) == 0) {
              strcpy(tempHead->val.val, inputs[i][2]);
            }
 	    //If not, add new Node with input as the KeyValue as the head, push the existing Node to next. 
	    else {
              push(&hashtable[hashVal], kv);
            } 
          }
          break;
	//When user invokes get command:  
        case 'g':
          if(inputs[i][1] == NULL || inputs[i][2] != NULL) {
		    printf("bad command\n");
		    break;
	      }
          hashVal = atoi(inputs[i][1]) % 100;
          Node *n = malloc(sizeof(Node));
          n = hashtable[hashVal];
	  //Iterate through the linked list at index = hash of the input key, to check if node with input key exists
          while( (atoi(n->val.key) != atoi(inputs[i][1])) && (atoi(n->val.key) != -1)) {
            n = n->next;
          }
	  //If end of linked list is reached, key not found
          if(atoi(n->val.key) == -1) {
            printf("%s not found\n", inputs[i][1]);
          }
	  //If node with key is found, display it
          else if(atoi(n->val.key) == atoi(inputs[i][1])) {
            printf("%s,%s\n", n->val.key, n->val.val);
          }
          break;
	//When user invokes delete command:  
        case 'd':
          if(inputs[i][1] == NULL || inputs[i][2] != NULL) {
		    printf("bad command\n");
		    break;
	      }
          hashVal = atoi(inputs[i][1]) % 100;
          Node *dNode = malloc(sizeof(Node));
          dNode = hashtable[hashVal];
          Node *pNode = malloc(sizeof(Node));
          strcpy(pNode->val.key,"-1");
          pNode->next = dNode;
	  //If node to be deleted is the first one in the list, move head of the linked list to next
          if(dNode != NULL && atoi(dNode->val.key) == atoi(inputs[i][1])){
            hashtable[hashVal] = dNode->next;
            break;
          }
	  //Else iterate through the linked list at index = hash of input key to find node just before the node to be deleted
          while( (atoi(dNode->val.key) != atoi(inputs[i][1])) && (atoi(dNode->val.key) != -1) ) {
            pNode = dNode;
            dNode = dNode->next;
          }
	  //Point the previous node to the next node
          if(atoi(dNode->val.key) == atoi(inputs[i][1])) {
            pNode->next = dNode->next;
          }
          free(dNode);
          break;
	//When user invokes clear command:
        case 'c':
          if(inputs[i][1] != NULL || inputs[i][2] != NULL) {
		    printf("bad command\n");
		    break;
	      }
	  //Empty the contents of the file
          fp = fopen("key_value_new.csv", "w");
          fclose(fp);
	  //Re-initialize the hashtable to contain just one node each with key as "-1"
          for(itr = 0; itr < 100; itr++) {
            Node *cNode = malloc(sizeof(Node));
            strcpy(cNode->val.key, "-1");
            hashtable[itr] = cNode;
          }
          break;
	//When user invokes all command:
        case 'a':
          if(inputs[i][1] != NULL || inputs[i][2] != NULL) {
		    printf("bad command\n");
		    break;
	      }
	  //Iterate through each of the hash values present
          for(itr = 0; itr < 100; itr++) {
            Node *aNode = malloc(sizeof(Node));
            aNode = hashtable[itr];
	    //Iterate through each linked list's nodes and display
            while(aNode != NULL && (atoi(aNode->val.key) != -1) ) {
              printf("%s,%s\n", aNode->val.key, aNode->val.val);
              aNode = aNode->next;
            }
          }
          break;
        default: 
          printf("bad command\n");
          break;
    }
  }

  //Re-write the whole file with the hashtable's contents
  fp =  fopen("key_value_new.csv", "w");
  for(itr = 0; itr < 100; itr++) {
    Node *currNode = hashtable[itr];
    while(currNode != NULL && atoi(currNode->val.key) != -1) {
      fprintf(fp, "%s,",currNode->val.key);
      fprintf(fp, "%s\n", currNode->val.val);
      currNode = currNode->next;
    }
  }
  fclose(fp);
}
