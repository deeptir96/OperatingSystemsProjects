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
  Node *hashtable[100] = {0};//malloc(sizeof(Node));// * 100;//{0};//{initKV, NULL};
  printf("Hellooo\n");
  for(i = 0; i < 100; i++) {
    Node *currN = malloc(sizeof(Node));
    strcpy(currN->val.key, "-1");
    hashtable[i] = currN;
    //strcpy(hashtable[i]->val.key, "-1");
    //printf("%s\n", hashtable[i].val.key);
  }

  //for(i = 0; i < 100; i++) {
    //strcpy(hashtable[i]->val.key, "-1");
    //printf("%s\n", hashtable[i]->val.key);
  //}

  fp = fopen("key_value_new.csv", "a+");
  int currHash = -1;
  char currKey[10];
  printf("again\n");
  while( getline(&line, &len, fp) > 1) {
    printf("while\n");
    printf("line %s\n", line);
    strcpy(currKey, line);
    char *token = strtok(currKey, ",");
    char *valToken = strtok(NULL, "\n");
    printf("Token = %s, valToken = %s\n", token, valToken);
    currHash = atoi(token) % 100;
    printf("while2\n");
    strcpy(kv.key, token);
    strcpy(kv.val, valToken);
    if(atoi(hashtable[currHash]->val.key) == -1) {
      printf("while if\n");
      push_front(&hashtable[currHash], kv);
    } else {
      printf("while else\n");
      Node *prev = malloc(sizeof(Node));
      Node *tempHead = malloc(sizeof(Node));
      tempHead = hashtable[currHash];
      printf("while else 1\n");
      int isTempHeadNull = 0;
      if(tempHead != NULL){
        isTempHeadNull = 1;
        printf("TempHead !NULL\n");
      }
      while(tempHead != NULL //&& tempHead->val != NULL
       && atoi(tempHead->val.key) != -1
       && strcmp(tempHead->val.key, inputs[i][1]) != -1) {
         printf("while else while\n");
         prev = tempHead;
         tempHead = tempHead->next;
      }
      if(tempHead != NULL //&& tempHead->val != NULL 
        && strcmp(tempHead->val.key, inputs[i][1]) == 0) {
         printf("while else if\n");
         strcpy(tempHead->val.val, inputs[i][2]);
      } else {
         printf("while else else\n");  
         push_front(&hashtable[currHash], kv);
      }
    }
  }
  fclose(fp);

  for(i = 1; i < argc; i++) {
    int k=0;
    while( (inp = strsep(&argv[i], ",")) != NULL) {
        if(k == 3) {
		printf("Invalid number of arguments/bad command");
		break;
		//Handle this somehow outside as well
	}
    	inputs[i][k++] = inp;
    }
    if(k > 3) {
     printf("Bad command\n");
    }
    char command = (char) inputs[i][0][0];
    int hashVal = atoi(inputs[i][1]) % 100;
    printf("Before command\n");
    switch(command) {
        case 'p':
          printf("Put 1\n");
          //KeyValue kv = {"-1", "Random"}; 
          //= { .key = atoi(inputs[i][1]), .val = inputs[i][2][0] };
          strcpy(kv.key, inputs[i][1]);
          strcpy(kv.val, inputs[i][2]);
          //printf
          if(/*hashtable[hashVal].val != NULL && */ atoi(hashtable[hashVal]->val.key) == -1) {
            printf("put if\n");
            push_front(&hashtable[hashVal], kv);
            //Node *putNode = malloc(sizeof(Node));
            //int success = putNode != NULL;
            //if ( success )
            //{
//              putNode->val = 348;
  //            putNode->next = hashtable[hashVal];
   //           hashtable[hashVal] = putNode;
            //}
            //else {
              //printf("Malloc errors\n");
            //}
          }
          else {
            printf("put else\n");
            Node *prev = malloc(sizeof(Node));
            Node *tempHead = malloc(sizeof(Node));
            tempHead = hashtable[hashVal];
            while(tempHead != NULL //&& tempHead->val != NULL
             && atoi(tempHead->val.key) != -1
             && strcmp(tempHead->val.key, inputs[i][1]) != 0) {
              prev = tempHead;
              tempHead = tempHead->next;
            }
            if(tempHead != NULL //&& tempHead->val != NULL 
             && strcmp(tempHead->val.key, inputs[i][1]) == 0) {
              strcpy(tempHead->val.val, inputs[i][2]);
            } else {
              push_front(&hashtable[hashVal], kv);
            }
            //Node *newVal = malloc(sizeOf(Node));
            //newVal->val = inputs[i][2][0];
            //newVal->next = tempHead;
          }
          printf("Values stored: %s\n", hashtable[hashVal]->val.key);
          //while(/*&hashtable[hashVal]->val != NULL &&*/ atoi(hashtable[hashVal]->val.key) != -1) {
            //printf("%s,%s\n",hashtable[hashVal]->val.key, hashtable[hashVal]->val.val);
         // }
        break;
        case 'g':
        break;
        case 'd':
        break;
        case 'c':
        break;
        case 'a':
        break;
        default: printf("Bad command\n");
        break;

    }
    
  }
  fp =  fopen("key_value_new.csv", "w");
  printf("Let's put in file\n");
  for(i = 0; i < 100; i++) {
    Node *currNode = hashtable[i];
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