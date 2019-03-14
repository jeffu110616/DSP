#!/usr/bin/env python
# coding: utf-8

# In[13]:


dt_mapping = {}

with open( "./Big5-ZhuYin.map", encoding='big5hkscs' ) as f:
    for line in f:
        # print( line.split(" ") )
        val = line.split(" ")[0]
        ls_z = [ w[0] for w in line.split(" ")[1].replace("\n", "").split("/") ]
        ls_z = list(set(ls_z))
        dt_mapping[val] = [val]
        for z in ls_z:
            try:
                dt_mapping[z].append( val )
            except KeyError:
                dt_mapping[z] = [ val ]
        
        


# In[17]:


with open( "./ZhuYin-Big5.map", "w", encoding='big5hkscs' ) as f:
    for k in dt_mapping:
        f.write( "{}\t{}\n".format( k, " ".join( dt_mapping[k] ) ) )
        


# In[ ]:





# In[ ]:




