a=int(input("enter a number: "))
if a<2:
    print(a," is not a prime number")
else:
    count=0
    for i in range(2,a):
        if a%i==0:
            count+=1
    if count==0:
        print(a," is a prime number")
    else:
        print(a," is not a prime number")
