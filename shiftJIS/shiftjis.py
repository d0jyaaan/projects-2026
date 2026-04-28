import csv

def main():
    filePath = "shiftJIS.txt"

    output = "test.txt"
    # 0x8140 to 0x9FFC
    # 0xE040 to 0xEBBF
    
    finalshiftjis = {}
    l1 = int("0x8140", 16)
    u1 = int("0x9FFC", 16)
    l2 = int("0xE040", 16)
    u2 = int("0xEBBF", 16)

    with open(output, "w") as out:
        with open(filePath, "r") as file:
            for line in file:
                splitted = line.split()

                shiftjis = int(splitted[0], 16)
                if shiftjis <= 0xFF:
                    continue

                if (l1 <= shiftjis <= u1 or l2 <= shiftjis <= u2):
                    if len(splitted[1]) == 6:

                        out.write(f"{splitted[0]} {splitted[1]}\n")

main()
