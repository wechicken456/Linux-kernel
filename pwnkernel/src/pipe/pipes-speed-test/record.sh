

for i in {1..10}; do
	sudo perf record -o write_read.data -g sh -c './write | ./read ' 2>/dev/null 1>>write_read.speed
done

for i in {1..10}; do
	sudo perf record -o write_vmsplice.data -g sh -c './write --write_with_vmsplice | ./read ' 2>/dev/null 1>>write_vmsplice.speed
done

for i in {1..10}; do
	sudo perf record -o read_splice.data -g sh -c './write | ./read --read_with_splice' 2>/dev/null 1>>read_splice.speed
done

for i in {1..10}; do
	sudo perf record -o vmsplice_splice.data -g sh -c './write --write_with_vmsplice  | ./read --read_with_splice' 2>/dev/null 1>>vmsplice_splice.speed
done

for i in {1..10}; do
	sudo perf record -o vmsplice_splice_dont_touch_page.data -g sh -c './write --write_with_vmsplice --dont_touch_page | ./read --read_with_splice' 2>/dev/null 1>>vmsplice_splice_dont_touch_page.speed
done

for i in {1..10}; do
	sudo perf record -o vmsplice_splice_huge_page.data -g sh -c './write --write_with_vmsplice --huge_page | ./read --read_with_splice' 2>/dev/null 1>>vmsplice_splice_huge_page.speed
done

for i in {1..10}; do
	sudo perf record -o vmsplice_splice_huge_page_busy_loop.data -g sh -c './write --write_with_vmsplice --huge_page --busy_loop | ./read --read_with_splice --busy_loop' 2>/dev/null 1>>vmsplice_splice_huge_page_busy_loop.speed
done



