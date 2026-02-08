#include <stdlib.h>
#include <stdio.h>

void swap(int* a, int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
}
int str_length(char *str){
    char*ptr = str;
    while(*ptr != '\0'){
        ptr++;
    }
    return ptr-str;
}

int main(){

    // Task 1
    int a = 5;
    int* ptr = &a;
    printf(".   // TASK 1 \n");
    printf("value of a using variable = %d \n", a);
    printf("value of a using pointer = %d \n", *ptr);
    *ptr = 50;
    printf("changed value of a = %d \n", a);


    // Task 2
    printf(".   // TASK 2 \n");
    int arr[5]={1,2,3,4,5};
    int *ptr1 = arr;
    for(int i=0; i<5; i++){
        printf("%d \n", *ptr1);
        ptr1++;
    }
    // Task 3
    printf(".   //TASK 3 \n");
    int x=4;
    int y=6;
    printf("value of x before swap = %d \n",x);
    printf("value of y before swap = %d \n", y);
    swap(&x,&y);
    printf("value of x after swap = %d \n", x);
    printf("value of y before swap = %d \n", y);

    // Task 4
    printf(".    //TASK 4 \n");
    int val= 10;
    int* ptr2= &val;
    int** ptr2Double= &ptr2;
    printf("value of val using pointer = %d \n", *ptr2);
    printf("value of val using double pointer = %d \n", **ptr2Double);

    // Task 5
    printf(".   //TASK 5 \n");
    int *n = malloc(sizeof(int));
    *n = 7;
    printf("value of n = %d \n",*n);
    int* arr1 = malloc(5*sizeof(int));
    int* ptr3 = arr1;
    for(int i=0; i<5; i++){
        *ptr3 = i + 1;
        ptr3++;
        printf("%d \n", arr[i]);
    }

    //Task 6
    printf(".   //TASK 6 \n");
    char *str = "String";
    char* charptr= str;
    while(*charptr != '\0'){
        printf("%c", *charptr);
        charptr++;
    }
    printf("\n");
    printf("Length of String is : %d \n", str_length(str));

    //Task 7
    printf(".   //TASK 7 \n");
    char *arr2[3]={"one","two","three"};
    printf("original array: \n");
    for(int i=0; i<3; i++){
        printf("%s\n",*(arr2+i));
    }
    arr2[1]="not two";
    printf("modified array: \n");
    for(int i=0; i<3; i++){
        printf("%s\n",*(arr2+i));
    }
}
