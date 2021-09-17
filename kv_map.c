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

/*int delete_node(Node **head, int key) {
  Node *dNode = malloc(sizeof(Node));
  dNode = *head;
  Node *pNode = malloc(sizeof(Node));
  int success = 0;
          //pNode = NULL;
          //strcpy(pNode->val.key,"-1");
          //pNode->next = dNode;
  if(dNode != NULL && atoi(dNode->val.key) == key) {
    *head = dNode->next;
  }
  while( dNode != NULL && (atoi(dNode->val.key) != key) && (atoi(dNode->val.key) != -1) ) {
    printf("Del while\n");
    pNode = dNode;
    dNode = dNode->next;
  }
  //TODO: add indentation
          if(dNode == NULL || atoi(dNode->val.key) == -1) {
            printf("%d not found\n", key);
          }
          else if(atoi(dNode->val.key) == key) {
            printf("Found key to delete\n");
            if(dNode->next == NULL) {
              printf("null dnode->next\n");
            } else {
              printf("okkayyy");
              printf("%s\n", dNode->next->val.key);
            }
            //means that the first node on list is to be deleted
          //  if(pNode == NULL) {
            //  hashtable[hashVal] = dNode->next;
            //} else {
            pNode->next = dNode->next;
            success = 1;
            //}
            //free(dNode);
            //printf("%s,%s\n", n->val.key, n->val.val);
          }
          free(dNode);
          return success;
}*/

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
  Node *hashtable[100] = {0};//malloc(sizeof(Node));// * 100;//{0};//{initKV, NULL};
  //printf("Hellooo\n");
  for(i = 0; i < 100; i++) {
    Node *currN = malloc(sizeof(Node));
    strcpy(currN->val.key, "-1");
    hashtable[i] = currN;
    //strcpy(hashtable[i]->val.key, "-1");
    //printf("%s\n", hashtable[i].val.key);
  }

  fp = fopen("key_value_new.csv", "a+");
  int currHash = -1;
  char currKey[10];

  //Gather inputs
  //printf("Starting over...\n");
  int k=0;

  while( getline(&line, &len, fp) > 1) {
    //printf("while\n");
      //printf("line %s\n", line);
      strcpy(currKey, line);
      char *token = strtok(currKey, ",");
      char *valToken = strtok(NULL, "\n");
      //printf("Token = %s, valToken = %s\n", token, valToken);
      currHash = atoi(token) % 100;
      //printf("Currhash = %d\n", currHash);
    //printf("while2\n");
      strcpy(kv.key, token);
      strcpy(kv.val, valToken);
      if(atoi(hashtable[currHash]->val.key) == -1) {
       
       push_front(&hashtable[currHash], kv);
       //printf("while if\n");
      } else {
          //printf("while else\n");
      //Node *prev = malloc(sizeof(Node));
          Node *tempHead = malloc(sizeof(Node));
          tempHead = hashtable[currHash];
    //      printf("while else 1\n");
      //int isTempHeadNull = 0;
          if(tempHead != NULL){
        //isTempHeadNull = 1;
    //        printf("TempHead !NULL\n");
          }
    //      printf("whatattt %s, token %s\n", tempHead->val.key, token);
          while(tempHead != NULL //&& tempHead->val != NULL
            && atoi(tempHead->val.key) != -1
       //changed from -1 to 0
           && strcmp(tempHead->val.key, token) != 0) {
         
    //        printf("while else while\n");
         //prev = tempHead;
            tempHead = tempHead->next;
          }
          //if(tempHead != NULL //&& tempHead->val != NULL 
           // && strcmp(tempHead->val.key, token) == 0) {
           // printf("while else if\n");
           // strcpy(tempHead->val.val, inputs[i][2]);
          //} else {
      //      printf("while else else\n");  
            push_front(&hashtable[currHash], kv);
          //}
      }
    }

    //printf("Finished reading file\n");
  
  fclose(fp);

  for(i = 1; i < argc; i++) {
    int j;
    for(j = 0; j < 3; j++) {
      inputs[i][j] = malloc(sizeof(char *));
      inputs[i][j] = NULL;
    }
  }

  for(i = 1; i < argc; i++) {
    //inputs[i] = malloc(sizeof(char))
    //printf("Arg number %d\n", i);
    //printf("Input %d: %s\n",i, argv[i]);
    int k=0;


    while( (inp = strsep(&argv[i], ",")) != NULL) {

      if(k == 3) {
		    printf("bad command");
		    break;
		//Handle this somehow outside as well
	    }
    	inputs[i][k] = inp;
      //printf("K = %d",k);
      k++;
    }
   // printf("Inppppss %s\n", inputs[i][0]);
      //printf("Inppppss %s\n", inputs[i][1]);
      //printf("Inppppss %s\n", inputs[i][2]);
    if(k > 3) {
     printf("bad command\n");
    }
    
    char command = (char) inputs[i][0][0];
    int hashVal;
    //printf("Fetching command %c\n", command);
    //printf("Before command\n");
    switch(command) {
        case 'p':
        //  printf("Putting");
          //printf("Before Put: inp1 %s, inp2 %s\n", inputs[i][1], inputs[i][2]);
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
          //printf
          if(/*hashtable[hashVal].val != NULL && */ atoi(hashtable[hashVal]->val.key) == -1) {
        
            push_front(&hashtable[hashVal], kv);
          
          }
          else {
            
            Node *tempHead = malloc(sizeof(Node));
            tempHead = hashtable[hashVal];
            while(tempHead != NULL //&& tempHead->val != NULL
             && atoi(tempHead->val.key) != -1
             && strcmp(tempHead->val.key, inputs[i][1]) != 0) {
              //prev = tempHead;
              tempHead = tempHead->next;
            }
            if(tempHead != NULL //&& tempHead->val != NULL 
             && strcmp(tempHead->val.key, inputs[i][1]) == 0) {
              strcpy(tempHead->val.val, inputs[i][2]);
            } else {
              push_front(&hashtable[hashVal], kv);
            }
            
          }
         // printf("Values stored: %s\n", hashtable[hashVal]->val.key);
          //while(/*&hashtable[hashVal]->val != NULL &&*/ atoi(hashtable[hashVal]->val.key) != -1) {
            //printf("%s,%s\n",hashtable[hashVal]->val.key, hashtable[hashVal]->val.val);
          //}
          break;
        case 'g':
          if(inputs[i][1] == NULL || inputs[i][2] != NULL) {
		        printf("bad command\n");
		        break;
	        }
          //TODO: add code for input correctness etc
          hashVal = atoi(inputs[i][1]) % 100;
          printf("");
          Node *n = malloc(sizeof(Node));
          n = hashtable[hashVal];
          while( (atoi(n->val.key) != atoi(inputs[i][1])) && (atoi(n->val.key) != -1)) {
          //   printf("Get while: node %s, inp %s\n", n->val.key, inputs[i][1]);
            n = n->next;
          }
          if(atoi(n->val.key) == -1) {
            printf("%s not found\n", inputs[i][1]);
          }
          else if(atoi(n->val.key) == atoi(inputs[i][1])) {
           //    printf("Value found, searching for key %s\n", inputs[i][1]);
           //  printf("Value is %s\n", n->val.key);
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
          //delete_node(&hashtable[hashVal], atoi(inputs[i][1]));
          Node *dNode = malloc(sizeof(Node));
          dNode = hashtable[hashVal];
          Node *pNode = malloc(sizeof(Node));
          //pNode = NULL;
          strcpy(pNode->val.key,"-1");
          pNode->next = dNode;
          if(dNode != NULL && atoi(dNode->val.key) == atoi(inputs[i][1])){
          //      printf("lololol\n");
            hashtable[hashVal] = dNode->next;
            break;
          }
          while( (atoi(dNode->val.key) != atoi(inputs[i][1])) && (atoi(dNode->val.key) != -1) ) {
          //      printf("Del while\n");
            pNode = dNode;
            dNode = dNode->next;
          }
          if(atoi(dNode->val.key) == -1) {
          //      printf("%s not found\n", inputs[i][1]);
          }
          else if(atoi(dNode->val.key) == atoi(inputs[i][1])) {
              //      printf("Found key to delete\n");
            if(dNode->next == NULL) {
            //        printf("null dnode->next\n");
            } else {
            //        printf("okkayyy");
            //         printf("%s\n", dNode->next->val.key);
            }
            //means that the first node on list is to be deleted
            //if(pNode == NULL) {
              //hashtable[hashVal] = dNode->next;
            //} else {
              pNode->next = dNode->next;
            //}
            //free(dNode);
            //printf("%s,%s\n", n->val.key, n->val.val);
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
            //strcpy(hashtable[itr2]->val.key, "-1");
            //printf("%s\n", hashtable[itr2].val.key);
          }
         // printf("Finished clearing\n");
          break;
        case 'a':
          if(inputs[i][1] != NULL || inputs[i][2] != NULL) {
		        printf("bad command\n");
		        break;
	        }
          //printf("printing all\n");
          int itr3;
          for(itr3 = 0; itr3 < 100; itr3++) {
            Node *aNode = malloc(sizeof(Node));
            aNode = hashtable[itr3];
            while(aNode != NULL && (atoi(aNode->val.key) != -1) ) {
              printf("%s,%s\n", aNode->val.key, aNode->val.val);
              aNode = aNode->next;
            }
          }
          //printf("Printed all");
          break;
        default: printf("bad command\n");
          break;
    }
    //printf("Finished switch\n");
  }
  fp =  fopen("key_value_new.csv", "w");
  //printf("Let's put in file\n");
  int itr4;
  for(itr4 = 0; itr4 < 100; itr4++) {
    Node *currNode = hashtable[itr4];
    //printf("currnode stuff: %s,%s\n", currNode->val.key, currNode->val.val);
    while(currNode != NULL && atoi(currNode->val.key) != -1) {
      //printf("%s,%s\n",currNode->val.key, currNode->val.val);
      fprintf(fp, "%s,",currNode->val.key);
      fprintf(fp, "%s\n", currNode->val.val);
      currNode = currNode->next;
    }
  }
  fclose(fp);
}