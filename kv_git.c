#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct KeyValue {
  char key[10];
  char val[30];
} KeyValue;

typedef struct Node {
    struct KeyValue val;
    struct Node *next;
} Node;

KeyValue keyVals = {
  .key = "-1"
};

int push_front( Node **head, KeyValue val)
{
    Node *new_node = malloc( sizeof( Node ) );
    int success = new_node != NULL;

    if ( success )
    {
        new_node->val = val;
        new_node->next = *head;
        *head = new_node;
    }

    return success;
}


int main(int argc, char *argv[]) {
  //Check num args etc
  int i;
  char *inp;
  char *inputs[argc][3];
  char value[30];
  FILE *fpr;
  FILE *fp2;
  FILE *fp;
  size_t len = 0;
  char *line = NULL;

  KeyValue kv = {.key = "-1"};
  Node *hashtable[100] = {0};
  for(i = 0; i < 100; i++) {
    Node *currN = malloc(sizeof(Node));
    strcpy(currN->val.key, "-1");
    hashtable[i] = currN;
  }

  fp = fopen("key_value_new.csv", "a+");
  int currHash = -1;
  char currKey[10];
  int k=0;

  while( getline(&line, &len, fp) > 1) {
    
      strcpy(currKey, line);
      char *token = strtok(currKey, ",");
      char *valToken = strtok(NULL, "\n");
      currHash = atoi(token) % 100;
      strcpy(kv.key, token);
      strcpy(kv.val, valToken);
      if(atoi(hashtable[currHash]->val.key) == -1) {
       
       push_front(&hashtable[currHash], kv);
       
      } else {
          Node *tempHead = malloc(sizeof(Node));
          tempHead = hashtable[currHash];
          if(tempHead != NULL){

          }
    
          while(tempHead != NULL
            && atoi(tempHead->val.key) != -1
           && strcmp(tempHead->val.key, token) != 0) {
            tempHead = tempHead->next;
          }
            
            push_front(&hashtable[currHash], kv);
          
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

  for(i = 1; i < argc; i++) {
    int k=0;


    while( (inp = strsep(&argv[i], ",")) != NULL) {

      if(k == 3) {
		    printf("bad command");
		    break;
		//Handle this somehow outside as well
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
        case 'p':
        
          if(inputs[i][1] == NULL || inputs[i][2] == NULL) {
		        printf("bad command\n");
		        break;
	        }
          int length = strlen(inputs[i][1]);
          int itr;
          for(itr = 0; itr < length; itr++) {
            if(!isdigit(inputs[i][1][itr])) {
              printf("bad command\n");
              break;
            }
          }
          
          hashVal = atoi(inputs[i][1]) % 100;
          strcpy(kv.key, inputs[i][1]);
          strcpy(kv.val, inputs[i][2]);
          
          if( atoi(hashtable[hashVal]->val.key) == -1) {
        
            push_front(&hashtable[hashVal], kv);
          
          }
          else {
            
            Node *tempHead = malloc(sizeof(Node));
            tempHead = hashtable[hashVal];
            while(tempHead != NULL 
             && atoi(tempHead->val.key) != -1
             && strcmp(tempHead->val.key, inputs[i][1]) != 0) {
            
              tempHead = tempHead->next;
            }
            if(tempHead != NULL 
             && strcmp(tempHead->val.key, inputs[i][1]) == 0) {
              strcpy(tempHead->val.val, inputs[i][2]);
            } else {
              push_front(&hashtable[hashVal], kv);
            }
            
          }
         
          break;
        case 'g':
          if(inputs[i][1] == NULL || inputs[i][2] != NULL) {
		        printf("bad command\n");
		        break;
	        }
          
          hashVal = atoi(inputs[i][1]) % 100;
          printf("");
          Node *n = malloc(sizeof(Node));
          n = hashtable[hashVal];
          while( (atoi(n->val.key) != atoi(inputs[i][1])) && (atoi(n->val.key) != -1)) {
            n = n->next;
          }
          if(atoi(n->val.key) == -1) {
            printf("%s not found\n", inputs[i][1]);
          }
          else if(atoi(n->val.key) == atoi(inputs[i][1])) {
            if(n->val.val == NULL) {
              printf("val is null\n");
            } else {

            }
            printf("%s,%s\n", n->val.key, n->val.val);
          }
          break;
        case 'd':
          if(inputs[i][1] == NULL || inputs[i][2] != NULL) {
		        printf("bad command\n");
		        break;
	        }
          printf("");
          hashVal = atoi(inputs[i][1]) % 100;
          Node *dNode = malloc(sizeof(Node));
          dNode = hashtable[hashVal];
          Node *pNode = malloc(sizeof(Node));
          strcpy(pNode->val.key,"-1");
          pNode->next = dNode;
          if(dNode != NULL && atoi(dNode->val.key) == atoi(inputs[i][1])){
            hashtable[hashVal] = dNode->next;
            break;
          }
          while( (atoi(dNode->val.key) != atoi(inputs[i][1])) && (atoi(dNode->val.key) != -1) ) {
            pNode = dNode;
            dNode = dNode->next;
          }
          if(atoi(dNode->val.key) == -1) {
          }
          else if(atoi(dNode->val.key) == atoi(inputs[i][1])) {
            if(dNode->next == NULL) {
            
            } else {
            
            }
              pNode->next = dNode->next;
          }
          free(dNode);
          break;
        case 'c':
          if(inputs[i][1] != NULL || inputs[i][2] != NULL) {
		        printf("bad command\n");
		        break;
	        }
          fp = fopen("key_value_new.csv", "w");
          fclose(fp);
          int itr2;
          for(itr2 = 0; itr2 < 100; itr2++) {
            Node *cNode = malloc(sizeof(Node));
            strcpy(cNode->val.key, "-1");
            hashtable[itr2] = cNode;
            
          }
          break;
        case 'a':
          if(inputs[i][1] != NULL || inputs[i][2] != NULL) {
		        printf("bad command\n");
		        break;
	        }
          int itr3;
          for(itr3 = 0; itr3 < 100; itr3++) {
            Node *aNode = malloc(sizeof(Node));
            aNode = hashtable[itr3];
            while(aNode != NULL && (atoi(aNode->val.key) != -1) ) {
              printf("%s,%s\n", aNode->val.key, aNode->val.val);
              aNode = aNode->next;
            }
          }
          break;
        default: printf("bad command\n");
          break;
    }
  }
  fp =  fopen("key_value_new.csv", "w");
  int itr4;
  for(itr4 = 0; itr4 < 100; itr4++) {
    Node *currNode = hashtable[itr4];
    while(currNode != NULL && atoi(currNode->val.key) != -1) {
      fprintf(fp, "%s,",currNode->val.key);
      fprintf(fp, "%s\n", currNode->val.val);
      currNode = currNode->next;
    }
  }
  fclose(fp);
}