#!/bin/bash

[[ $# -eq 1 ]] || { echo "Usage: $0 filename"; exit 1; }

mkdir -p word
cp $1 a.docx
unzip -p $1 word/document.xml \
	|sed 's/{/\&lt2;/g' \
	|sed 's/}/\&rt2;/g' \
	|awk '{
			while (match($0, /<w:r>/)) {
				s = substr($0, RSTART, RLENGTH)
				$0 = substr($0, 1, RSTART-1) "\n" "{w:r}" "\n" substr($0, RSTART+RLENGTH)
			}
			print
		}'\
	|awk '{
			while (match($0, /<((w:t)|(w:rPr))>/)) {
				s = substr($0, RSTART, RLENGTH)
				$0 = substr($0, 1, RSTART-1) "\n" "{" substr(s, 2, RLENGTH-2) "}" substr($0, RSTART+RLENGTH)
			}
			print
		}'\
	|awk '{
			while (match($0, /<\/((w:t)|(w:rPr))>/)) {
				s = substr($0, RSTART, RLENGTH)
				$0 = substr($0, 1, RSTART-1) "{" substr(s, 2, RLENGTH-2) "}" "\n" substr($0, RSTART+RLENGTH)
			}
			print
		}'\
	|sed 's/{/</g' | sed 's/}/>/g' \
	|sed 's/\&lt2;/{/g' \
	|sed 's/\&rt2;/}/g' \
	|awk '/<w:t>.+<\/w:t>/ { gsub(/ /, "</w:t>\n<w:t xml:space=\"preserve\"> ") } { print }' \
	|awk '/<w:r>/ { rPr = "" }
	      /<w:rPr>.+<\/w:rPr>/ { rPr = $0 }
	      /<w:t.+<\/w:t>/ { count++ } count == 3 && /<w:t.+<\/w:t>/ { $0 = "</w:r><w:r><w:rPr><w:b/><w:bCs/></w:rPr>" $0 "</w:r><w:r>" rPr } { print }' \
	|sed ':a; N; $!ba; s|</w:t>\n<w:t xml:space="preserve">||g' \
	>word/document.xml
	zip -r a.docx word/document.xml

# xdg-open a.docx
