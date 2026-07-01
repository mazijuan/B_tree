import docx

doc = docx.Document(r'd:\代码\06\2026年春季学期-软件项目进阶实践选题目录-李宽.docx')

with open(r'd:\代码\06\projects.txt', 'w', encoding='utf-8') as f:
    for i, para in enumerate(doc.paragraphs):
        if para.text.strip():
            f.write(f"{i+1}. {para.text}\n\n")

print("Done!")
