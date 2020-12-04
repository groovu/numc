import numc as nc
import dumbpy as dp
import pdb
a = nc.Matrix(2,2,[1.1,2.2,3.3,-4.4])
b = nc.Matrix(1,3,[1.1,2,-3])
db = dp.Matrix(1,3,[1,2,-3])
c = nc.Matrix(2,3,[1,2,3,4,5,6])
e = nc.Matrix(1,2,[1,2])
f = nc.Matrix(1,1,[3])
g = nc.Matrix(2,2,[1,2,3,-4])
da = dp.Matrix(2,2,[1.1,2.2,3.3,-4.4])
a.get(0,0)
# pdb.set_trace()
b[0]
a[1]
#a[0:0]
#a[0,0]
db[-1:]
