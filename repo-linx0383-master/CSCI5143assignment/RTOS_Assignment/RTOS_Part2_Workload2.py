#CSCI5143 RTOS Assignment Part2_Workload2
from math import ceil

#Input of the period set (Workload2)
period_List=[20,30,50]
ET_List=[5,12,15]
deadline_List=[20,30,50]
bi_ss=[1,0,0]
ki=1 #number of times job of Ti can self-suspend
bi_np=[0,0,0]

bi=[0,0,0]
bk_ss=0
N=len(period_List)
sum1=0
sum2=0
blocking_time=0
w=[0]
w2=[0]
response=[]
s=[]
print "Workload2"
for i in range(N):
  print "Task%d: Period:%d  Deadline:%d  Execution Time:%d" %(i+1,period_List[i],deadline_List[i],ET_List[i])

i=0
for i in range(N):
    w.append(0)
    w2.append(0)
    response.append([])
    s.append([])

i=0
k=0
j=0

for j in range(N):
    s[j]=deadline_List[j]
deadline_List.sort()
j=0
p=0
for j in range(N): #change the order of the tasks based on deadline monotonic priority
        while s[j]!=deadline_List[p]:
                p=p+1
        if(j!=p):
            store_value=period_List[j]
            period_List[j]=period_List[p]
            period_List[p]=store_value

            store_value=ET_List[j]
            ET_List[j]=ET_List[p]
            ET_List[p]=store_value

            store_value=bi_ss[j]
            bi_ss[j]=bi_ss[p]
            bi_ss[p]=store_value

            store_value=bi_np[j]
            bi_np[j]=bi_np[p]
            bi_np[p]=store_value

            store_value=s[j]
            s[j]=s[p]
            s[p]=store_value
        p=0
        
j=0
for i in range(N):
    while k<=(i-1):
        bk_ss=bk_ss+bi_ss[k]
        k=k+1
    k=0
    bi[i]=bi_ss[i]+bk_ss+(ki+1)*bi_np[i]
    bk_ss=0

i=0
k=0

for i in range(N):
    t=ET_List[i]
    while 1:
        while k<=(i-1):
              sum2=sum2+ceil(t/period_List[k])*ET_List[k]
              k=k+1
        k=0
        w[i]=ET_List[i]+sum2+bi[i]
        sum2=0
        t_nest=w[i]
        while k<=(i-1):
              sum2=sum2+ceil(t_nest/period_List[k])*ET_List[k]
              k=k+1
        k=0
        w2[i]=ET_List[i]+sum2+bi[i]
        sum2=0
        if w2[i]==w[i]:
            response[i]=w2[i]
            print "Response time for w%d is %d. Blocking time is %d" %((i+1),w2[i],bi[i])
            if w2[i]<=deadline_List[i]:
                print "It meets deadline"
            else:
                print "It misses deadline"
            break
        else:
            t=w2[i]
