import numc as nc
import dumbpy as dp
import pdb
a = nc.Matrix(2,2,[1.1,2.2,3.3,-4.4])
b = nc.Matrix(1,3,[1.1,2,-3])
db = dp.Matrix(1,3,[1,2,-3])
c = nc.Matrix(3,3,[1,2,3,4,5,6,7,8,-9.9])
dc = dp.Matrix(3,3,[1,2,3,4,5,6,7,8,-9.9])
e = nc.Matrix(1,2,[1,2])
f = nc.Matrix(1,1,[3])
g = nc.Matrix(2,2,[1,2,3,-4])
da = dp.Matrix(2,2,[1.1,2.2,3.3,-4.4])
a.get(0,0)
# pdb.set_trace()
# b[0]
# a[1]
# #a[0:0]
# #a[0,0]
# db[-1:]
# a[0:1] = [1,1]
# c[0:2,1]












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

