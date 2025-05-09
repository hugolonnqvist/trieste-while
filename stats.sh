#!/bin/bash

# This ugly script runs the partial and full randomization generation pyton script 
# and following data runs the Trieste parser and static analysier on it
# Most of the ugly commands are to easy format the output for latex
# Feel free reformat this

function gather_data() {
	cat tmp.txt | awk '!/Starting/ && (/INST POST NORM:/ || /VARS POST NORM:/ || /functions/ || /expressions/ || /statements/ || /check_refs/ || /unique_variables/ || /gather/ ||/constant_folding/ || /dead_code_el/) ' \
		| awk '{
			if ($1 == "constant_folding") cf += $4;
			else if ($1 == "dead_code_elimination") dce += $4;
			else if ($1 == "functions" || $1 == "expressions" || $1 == "statements" || $1 == "check_refs" || $1 == "unique_variables") pt += $4;
			else if ($1 == "gather_functions" || $1 == "gather_instructions" || $1 == "gather_flow_graph") cfg += $4;
			else if ($1 == "VARS") vars += $4;
			else if ($1 == "INST") inst += $4;
			}
		END {
			print "INST ", int(inst / 25);
			print "VARS ", int(vars / 25);
			print "PT ", int((pt / 1000) / 25);
			print "CFG ", int((cfg / 1000) / 25);
			print "CP ", int((cf / 1000) / 25);
			print "DCE ", int((dce / 1000) / 25);
		}' >> stats_result.txt
}


loc=(100 250 500 750 1000 1250 1500 1750 2000)
# loc=(1000)

header="LOC Instructions Vars Parsing Control-Flow Constant-Prop Live/Dead end"

echo "Running partial generation"
echo "Partial mode" > stats_result.txt
echo $header >> stats_result.txt
for i in ${loc[@]}; do
	echo "" > tmp.txt
	echo "With -loc = $i"
	echo "$i " >> stats_result.txt
	for j in {1..25}; do
		echo "round: $j"
		./program_generation -loc $i -p true >> tmp.txt
	done
	gather_data
	echo "end" >> stats_result.txt
done

echo "Running full generation"
echo "Full mode" >> stats_result.txt
echo $header >> stats_result.txt
for i in ${loc[@]}; do
	echo "" > tmp.txt
	echo "With -loc = $i"
	echo "$i " >> stats_result.txt
	for j in {1..25}; do
		echo "round: $j"
		./program_generation -loc $i -f true >> tmp.txt
	done
	gather_data
	echo "end" >> stats_result.txt
done


# Formatting for latex
sed -i -e 's/\(CP\|DCE\|PT\|CFG\|VARS\|INST\) //'  -e 's/end/\\\\/'  ./stats_result.txt
tr -d '\n'  < stats_result.txt | sed -e 's/\\\\\|mode/\n/g'> tmp.txt && cat tmp.txt > stats_result.txt

rm tmp.txt
cat stats_result.txt
