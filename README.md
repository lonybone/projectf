# projectf

**WICHTIG: Die Kompilation ist derweil nur für Linux basierte Systeme ausgelegt**

Dieses Projekt enstand aus Eigeninitiave aufgrund der Faszination gegenüber der hardwarenahen Programmierung.<br /> Aus diesem Grund ist natürlich der gesamte funktionale Code in C geschrieben, welche die höchste hardwarenähe nach Assembler für die meisten Systeme bietet.

## Outline
- Erstellen eines Tokenizers, der eine Datei an Zeichen in einen Tokenstream übersetzt. (lexer.c)
- Transformieren des eindimensionalen Tokenstreams in einen abstrakten Syntaxbaum (parser.c)
- Überprüfen des abstrakten Syntaxbaumes auf Fehler in Bereich Datentypen, Argumenten, Parametern etc.. Also eine semantische Analyse. (typeChecker.c)
- Übersetzen des abstrakten Syntaxbaumes in NASM-Assembler Sprache. (codegen.c)
- Übersetzung der generierten Assembler Datei in nativen Maschinencode. (main.c)

-> Nach all diesen Schritten wird eine Datei mit demselben Namen wie die Inputdatei erstellt. Diese kann dann einfach per Konsole ausgeführt werden. <br />Zum jetzigen Zeitpunkt wird immer der letzte Wert im CPU-Register "RAX" als Ganzzahl interpretiert ausgegeben, nachdem das Programm das Ende der Methode "main" erreicht hat.


## Limitationen
- Nur Ganzzahlen und Wahrheitswerte sind valide Datentypen
- Keine Gleitkommazahlen, sowie Strings oder Characters
- Keine Klassen oder Objektorientierte Programmierung
- Keine dynamische Speicher Zuweisung

## Ausblick
- Implementation aller oben genannten Limitationen ist in Planung. Jedoch sind alle Punkte davon sehr schwierig zu Konzepieren und werden demnach nur Etappenweise implementiert werden können.
- Die größte Designfrage, die Art der synamischen Speicherzuweisung, steht noch aus. Dies ist die mit Abstand kritischste Entscheidung, da basierend darauf nicht nur die Performance sondern auch der ganze Workflow beeinflusst wird.
