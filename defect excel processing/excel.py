import xlsxwriter
import pandas as pd
import re

def main():
    workbook = xlsxwriter.Workbook('Defect_Processed.xlsx')
    worksheet = workbook.add_worksheet()

    df = pd.read_excel("Defect.xlsx")
    
    for i, row in enumerate(df.values):
        for j, value in enumerate(row):

            if not pd.isna(value):
                if (j < 3):
                    worksheet.write(i, j, value)
                else:
                    delimiters = r"[,.]"
                    k = 0
                    for op in (re.split(delimiters, value)):
                        # print(op)
                        if not (op == ""):
                            worksheet.write(i, j + k + 1, op.strip())
                            k += 1


    workbook.close()

main()
