table = []

for i in range(1 << 8):
	ternary = 0
	for j in range(8):
		ternary *= 3
		if i & (1 << (7 - j)):
			ternary += 1
	table.append(ternary)

print(table)
