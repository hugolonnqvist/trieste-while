#!/bin/bash

# This ugly script runs the partial and full randomization generation pyton script 
# and following data runs the Trieste parser and static analysier on it
# Most of the ugly commands are to easy format the output for latex
# Feel free reformat this

function gather_data() {
	cat tmp.txt | grep ms >> stats_result.txt
	cat tmp.txt | awk '!/Starting/ && (/constant_folding/ || /dead_code_eli/)' \
		| awk '{
			if ($1 == "constant_folding") cf += int($4 / 1000);
			else if ($1 == "dead_code_elimination") dce += int($4 / 1000)
			}
		END {
			print "CP &", cf;
			print "DCE &", dce
		}' >> stats_result.txt
}


loc=(50 100 250 500 1000)

echo "Running partial generation"
echo "Partial gen " > stats_result.txt
for i in ${loc[@]}; do
	echo "With -loc = $i"
	echo "& $i" >> stats_result.txt
	./program_generation -loc $i -p true > tmp.txt
	gather_data
	echo "end" >> stats_result.txt
done

echo "Running full generation"
echo "Full generation " >> stats_result.txt
for i in ${loc[@]}; do
	echo "With -loc = $i"
	echo "& $i" >> stats_result.txt
	./program_generation -loc $i -f true > tmp.txt
	gather_data
	echo "end" >> stats_result.txt
done


# Formatting for latex
sed -i -e 's/\(Parse\|Static\)[a-z ():]\+/ \& /' -e 's/\(CP\|DCE\) //'  -e 's/end/\\\\/' -e 's/ms/ /' -e 's/[0-9]\+/\\num{&}/g' ./stats_result.txt
tr -d '\n'  < stats_result.txt | sed -e 's/\\\\/ \\\\\n/g' > tmp.txt && cat tmp.txt > stats_result.txt

rm tmp.txt
