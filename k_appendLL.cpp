#include<iostream>
using namespace std;
class node
{
    public:
    int data;
    node* next;
    node(int d)
    {
        data=d;
        next=NULL;
    }
};
void insertAtTail(node* &head ,int data)
{
    if(head==NULL)
    {
        head=new node(data);
        return;
    }
    node* tail=head;
    while(tail->next!=NULL)
    {
        tail=tail->next;
    }
    tail->next =new node(data);
    return;
}
void buildlist(node *&head)
{
  int N;
  cin>>N;
  int data;
  while(N>0)
  {
    cin>>data;
    insertAtTail(head,data);
    N--;
  }
}
void display(node *c)
{
    while(c!=NULL)
    {
        //cout<<"hooo";
        cout<<c->data<<" ";
        c=c->next;        
    }
}
void K_append(int k,int n ,node* &head)
{
    node*prev=head;
    node* curr=head;
    int count=1;
    curr=curr->next;
    while(count<=n-k)
    {
        //curr=curr->next;
        prev=curr;
        curr=curr->next;
    }prev->next=NULL;
   // head=curr;
    node*temp =curr;
    //count=0;
    while(temp->next!=NULL)
    {   

        temp=temp->next;
        //count++;
    }temp->next=head;
    head=curr;

}
int main() {
    int n;
    node* head=NULL;
    cin>>n;e: cannot open output file E:\CODING_BLOCKS/k_appendLL.exe: Permission denied
collect2.exe: error: ld returned 1 exit status
[Finished in 0.4s with exit code 1]
[shell_cmd: g++ "E:\CODING_BLOCKS\k_appendLL.cpp" -o "E:\CODING_BLOCKS/k_appendLL" && "E:\CODING_BLOCKS/k_appendLL"]
[dir: E:\CODING_BLOCKS]
[path: C:\Program Files (x86)\Intel\iCLS Client\;C:\Program Files\Intel\iCLS Client\;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files (x86)\Intel\Intel(R) Management Engine Components\DAL;C:\Program Files\Intel\Intel(R) Management Engine Components\DAL;C:\MinGW\bin;C:\Program Files\nodejs\;C:\Program Files\Git\cmd;C:\Users\ESHANIKA\AppData\Local\Microsoft\WindowsApps;C:\Users\ESHANIKA\AppData\Roaming\npm;C:\Users\ESHANIKA\AppData\Local\Programs\Microsoft VS Code\bin]
    buildlist(head);
    int k;
    cin>>k;

    K_append(k,n,head);
    display(head);
	return 0;
}