#CSCI5143 RTOS Assignment Part1_Workload2
#import lcm

#Input of the period set (Workload2)
period_List=[20,20,30,30,50,50,50,50]
WCET_List=[4,1,2,1,1,1,2,5]
offset_List=[0,5,5,5,10,10,25,25]
deadline_List=[15,20,30,30,40,40,50,50]

next_release_time_List=offset_List
Next_deadline_List=deadline_List
period_List.sort()
deadline_List.sort()

time=0 #initialize time
N=len(period_List) # number of tasks
i=0
j=0

run=[0]
rem=[0]
Number=list()
WCET=list()
offset=list()
deadline=list()
    

for i in range(N):
    run.append(0)
    rem.append(0)

    Number.append([])
    WCET.append([])
    offset.append([])
    deadline.append([])

def LCM(U):  #define largest common multiple for an array
    dumb=[0]
    for i in range(len(U)):
        dumb.append(0)
    for i in range(1,len(U),1):
       if U[i] >= U[i-1]:
           greater = U[i]
       else:
           greater = U[i-1]
    while(1):
        sumdumb=0
        for i in range(0,len(U),1):
            dumb[i]=greater % U[i]
        for i in range(0,len(U),1):
            sumdumb=sumdumb+dumb[i]
        if sumdumb==0:
            lcm=greater
            i=0
            break
        greater += 1
    return lcm

Plcm=LCM(period_List)#the value of LCM for the period task set

while time<Plcm:
    last_time=time
    turnover=next_release_time_List[N-1]
    for g in range(0,N,1):
        if time==next_release_time_List[g]:
            time=next_release_time_List[g]
            j=g
            break
    if time>=next_release_time_List[j]:
       print "The starting   time of task",j+1," is ",time
       time=time+WCET_List[j]
       print "The completion time of task",j+1," is ",time
       if time<Next_deadline_List[j]:
          print "No missed deadline"
          Next_deadline_List[j]=Next_deadline_List[j]+period_List[j]
       else:
          print "Missed deadline!!!"
       next_release_time_List[j]=next_release_time_List[j]+period_List[j]
       if j<(N-1):
           j=j+1
       last_j=j
    else:
        j=j+1
    if j==N:
        time=time+1
        j=last_j
        for w in range(0,N,1):
            if time>=next_release_time_List[w]:
                time=next_release_time_List[w]
                j=0
    if turnover!=next_release_time_List[N-1]:
       j=0
    for k in range(0,N,1):
        rem[k]=time%period_List[k]
       # print "rem",rem[k]
        if rem[k]==0 and time!=last_time:
            j=0

