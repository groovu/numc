import numc as nc
import dumbpy as dp
import pdb
import random
import utils
a = nc.Matrix(17,5,3.3)
da = dp.Matrix(17,5,3.3)
b = nc.Matrix(5,17,9)
db = dp.Matrix(5,17,9)

c = nc.Matrix(3,3,[1,2,3,4,5,6,7,8,-9.9])
dc = dp.Matrix(3,3,[1,2,3,4,5,6,7,8,-9.9])
e = nc.Matrix(1,2,[1,2])
f = nc.Matrix(1,1,[3])
g = nc.Matrix(2,2,[1,2,3,-4])
a.get(0,0)
# pdb.set_trace()
# b[0]
# a[1]
# #a[0:0]
# #a[0,0]
# db[-1:]
# a[0:1] = [1,1]
# c[0:2,1]










# print("mul check")
# print("b*c == db * dc", b*c == db*dc)

#set sanity
z = nc.Matrix(2,2,[1.1,2.2,3.3,-4.4])
x = dp.Matrix(2,2,[1.1,2.2,3.3,-4.4])
print("z:", z, "\nx:",x)
print("z[0:1]== x[0:1]", z[0:1] == x[0:1])
print("z[0:2] == x[0:2]",z[0:2] == x[0:2])
print("z+z == x+x", z+z == x+x)

z = nc.Matrix(3,3,[1,2,3,4,5,6,7,8,-9.9])
x = dp.Matrix(3,3,[1,2,3,4,5,6,7,8,-9.9])
print("z:", z, "\nx:",x)
print("z[0:1]== x[0:1]", z[0:1] == x[0:1])
print("z[0:2] == x[0:2]",z[0:2] == x[0:2])
print("z[0:2,1]==x[0:2,1]", z[0:2,1]==x[0:2,1])
twoBtwo = [1,1]
z[0:2,1] = twoBtwo 
x[0:2,1] = twoBtwo
print("update to", twoBtwo)
print("z[0:2,1]==x[0:2,1]", z[0:2,1]==x[0:2,1])
print("z[0:2,0:1] == x[0:2,0:1]", z[0:2,0:1] == x[0:2,0:1])

rando_list = []
rows = 50
cols = 5000
for i in range(0,rows * cols):
    n = random.uniform(0,100000)
    rando_list.append(n)

z = nc.Matrix(rows, cols, rando_list)
dz = dp.Matrix(rows, cols, rando_list)
y = nc.Matrix(cols, rows, rando_list)
dy = dp.Matrix(cols, rows, rando_list)

rando_list2 = []
for i in range(0, rows  * rows ):
    rando_list2.append(random.uniform(0,100000))

x = nc.Matrix(rows , rows , rando_list2)
dx = dp.Matrix(rows , rows , rando_list2)

# print(z*y == dz*dy)
# print(y*z == dy*dz)

print("is x == dx?", x == dx)


for i in range(0, 10):
    if (x**i != dx**i):
        print(i)

c = nc.Matrix(1,2,1)
d = nc.Matrix(2,1,1)

