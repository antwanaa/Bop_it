import wave
import argparse


def main():
	parser = argparse.ArgumentParser(description="Generate a C audio sample array in a header file")
	parser.add_argument("--file", "-f", type=str, default=None, help="The imput .wav audio file")
	parser.add_argument("--output", "-o", type=str, default=None, help="The output .h file")

	args = parser.parse_args()

	if (args.file == None):
		print("Error: please provide an input .wav file")
	if (args.output == None):
		print("Error: please provide an output .h file")

	name = args.output[:-2]

	print("Opening {}".format(args.file))
	file = wave.open(args.file, "r")
	length = file.getnframes()
	samples = []

	print("num samples: {}".format(length))
	print("Reading samples")

	for i in range(0, length):
		sample = file.readframes(1)
		samples.append(sample[0])

	print("Generating .h file")
	out_file = open(args.output, "w")

	out_file.write("/* File autogenerated using the sampler.py script using the {} audio file */\n".format(args.file))
	out_file.write("\nuint16_t {}_sample_length = {};\n".format(name, length))
	out_file.write("\nconst uint8_t {}_sample[] = ".format(name) + "{\n\t")

	i = 0
	for s in samples:
		out_file.write("{},".format(s))
		if (i == 25):
			out_file.write("\n\t")
			i = 0
		i += 1

	out_file.write("\n\t};\n")

	file.close()
	out_file.close()

	print("Done")




if __name__ == "__main__":
	main()