import sys,os

def main(arg):
    for i in arg:
        if not i.endswith(".sst"):
            continue
        f = open(i, 'rb')
        f2 = open("../Debug_Tools/Debug/"+i[:-4]+"-result.txt", 'w+')
        b = f.read()

        tim = int.from_bytes(b[0:8],byteorder='little',signed=False)
        print("Time:", tim)
        size = int.from_bytes(b[8:16],byteorder='little',signed=False)
        f2.write("Time: " + str(tim) + "\n")
        print("Size:", size)
        f2.write("Size: " + str(size) + "\n")
        maxi = int.from_bytes(b[16:24],byteorder='little',signed=False)
        print("Max:", maxi)
        f2.write("Max: " + str(maxi) + "\n")
        mini = int.from_bytes(b[24:32],byteorder='little',signed=False)
        print("Min:", mini)
        f2.write("Min: " + str(mini) + "\n")

        #print("BF:", int.from_bytes(b[32:10272],byteorder='little',signed=False))
        #print("Index:")
        content = b[:]
        b = b[10272:]
        offsetArray = b[(size+1)*8:]

        i = 0
        while i < size:
            key = int.from_bytes(b[i * 8:(i+1) * 8],byteorder='little',signed=False)
            #print("key:", key, end=', ')
            offset = int.from_bytes(offsetArray[i * 4:(i+1) * 4],byteorder='little',signed=False)
            nextoffset = int.from_bytes(offsetArray[(i+1) * 4:(i+1) * 8],byteorder='little',signed=False)
            #print("offset:", offset)
            try:
                f2.write("key: "+str(key)+", offset: "+ str(offset) + ", content: " +content[offset:nextoffset].decode()+"\n")
            except UnicodeDecodeError:
                f2.write("Key: "+str(key)+", offset: "+ str(offset) + ", content: " +str(content[offset:nextoffset])+"\n")
            #print("value:", content[offset:nextoffset])
            i += 1
        f.close()
        f2.close()

if __name__=="__main__":
    if len(sys.argv) > 1:
        main(sys.argv[1:])
    else:
        main(os.listdir("."))
