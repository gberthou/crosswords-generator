import sys

# TODO: Import definitions from external file

# 1
#defsH = [["A l'aise plaisent"], ["Passait de bons maux membres (s')"], ["A cuit t\\^ete", "Partie de s\\'eminaire"], ["Elle retrouve le nectar vers miel", "Tranche de cr\\^epe"], ["Herbe y haies", "Dirham familial"], ["Loupa Capitolina", "Jaune \\`a l'id\\'ee"], ["Arriv\\'e en t\\^ete", "Allant se faire voix"], ["Petit veut rien"], ["Autour du pingre noble"], ["Fronti\\`eres du Canada", "Sc\\`ene de m\\'eninges"], ["Cimes agr\\`es"]]
#
#defsV = [["Il va sang charg\\'e"], ["Bigouden aux pommes", "En forme rompt l'originalit\\'e"], ["Doser pas des eaux", "Par dix !"], ["Bleu scl\\`ere", "Gard\\^at au vrai"], ["Capital de la peine", "En pointes y est"], ["Y r\\'ealisas able", "Elle aime atomes"], ["S\\^ur en cherra", "Compos\\'ee organique"], ["Baie de Charente", "Son lad Cesar"], ["Elles se coupent les cheveux en trois"]]

# 2
defsH = [["T'es d'X INRIA"], ["Ils est ont"], ["Anti qu'est presto", "Charbon d\\'eboire"], ["Il rafra\\^ichit l'al\\`ene", "Ou\\\"ie film"], ["Pieds de porc", "Petit homme verre", "On y voit gouttes"], ["Moutarde de disjoncte", "Sirop pour le cou"], ["Jaune pousse", "Poulie technique"], ["\\`A t\\^ete de Turcs", "Elle descend du cygne (phon.)"], ["Gras pillage", "Quinquina de eau lande"], ["Frais de notoire", "Carbone \\`a ras"], ["Sales attentes"]]

defsV = [["Au soleil o\\`u l\\`a cuisisse"], ["Ils logent place de la bourse"], ["Pass\\'e cr\\`eme", "Semence tenante"], ["R\\'eput\\'e des insoumis", "Fa\\^ite d'abaca", "L\\`a chut du murmure"], ["Contenu d'un coffre au t\\'enor", "En cool devient ivre", "``To be'' au Mans"], ["Chef d'Italie", "Pass\\'e La Verpilli\\`ere"], ["Mak\\^om bat le tamb\\^ur", "Komi d'office", "Pas mieux que la devise d'Espagne"], ["Provoque un raz-de-mar\\'ee aux pores", "Hache des airs"], ["INSA valeurs"]]

roman = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI"]

def write_tex_header():
    print(r"""\documentclass[11pt,a4paper]{memoir}
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage[pdftex]{graphicx}
\usepackage{enumitem}
\usepackage[french]{babel}
\linespread{1}
\usepackage[a4paper, lmargin=0.1666\paperwidth, rmargin=0.1666\paperwidth, tmargin=0.1111\paperheight, bmargin=0.1111\paperheight]{geometry}
\usepackage[all]{nowidow}
\usepackage[protrusion=true,expansion=true]{microtype}
\usepackage{tikz}
\begin{document}""")
def write_tex_footer():
    print(r"\end{document}")

with open(sys.argv[1], "r") as f:
    width = 0
    height = 0

    write_tex_header()

    print("\\begin{tikzpicture}")
    for i,line in enumerate(f):
        height += 1
        for j,c in enumerate(line):
            if ord(c) < ord('a') or ord(c) > ord('}'):
                break
            if i == 0:
                width += 1

            command = ["draw", "fill"][c == "{"]
            print("\\%s (%d,%d) rectangle (%d,%d);" % (command, j, -i, j+1, -(i+1)))

    for i in range(width):
        print("\\node at(%.1f,0.5) {\\huge \\textbf{%d}};" % (i+.5, i+1))

    for i in range(height):
        print("\\node[anchor=east] at(0,%.1f) {\\huge \\textbf{%s}};" % (-i-.5,roman[i]))

    print("\\end{tikzpicture}")
    print("\n\\bigskip\n")
    print("\n\\Large\n")

    print("\\begin{center}\\textbf{HORIZONTALEMENT}\\end{center}\n")
    print("\\noindent")
    textH = []
    for i,d in enumerate(defsH):
        line = ". ".join(d)
        textH.append("\\textbf{%s.}~%s" % (roman[i], line))
    print(" --\n".join(textH) + ".")
    print("\n\\bigskip\n")
    print("\\begin{center}\\textbf{VERTICALEMENT}\\end{center}\n")
    print("\\noindent")
    textV = []
    for i,d in enumerate(defsV):
        line = ". ".join(d)
        textV.append("\\textbf{%s.}~%s" % (i+1, line))
    print(" --\n".join(textV) + ".\n")

    write_tex_footer()
