/*
* CSCI3280 Introduction to Multimedia Systems *
* --- Declaration --- *
* I declare that the assignment here submitted is original except for source
* material explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/ *
* Assignment 3
* Name : Lee Kong Yau
* Student ID :1155149600
* Email Addr :1155149600@link.cuhk.edu.hk
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define CODE_SIZE  12
#define TRUE 1
#define FALSE 0




/* function prototypes */
unsigned int read_code(FILE*, unsigned int);
void write_code(FILE*, unsigned int, unsigned int);
void writefileheader(FILE*, char**, int);
void readfileheader(FILE*, char**, int*);
void compress(FILE*, FILE*);
void decompress(FILE*, FILE*);

class Node {
public:
	std::string word;
	Node* next;
};

void append(Node** head, std::string words) {
	Node* new_node = new Node();
	Node* last = *head;

	new_node->word = words;
	new_node->next = NULL;
	if (*head == NULL) {
		*head = new_node;
		return;
	}
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = new_node;
	return;
}

int search(Node* head, std::string x)
{
	int a = 0;
	Node* current = head;
	while (current != NULL)
	{
		if (current->word == x) {
			return a;
		}
		current = current->next;
		a++;
	}
	return a;
}

void deleteNode(Node** head, int position)
{
	if (*head == NULL)
		return;
	Node* temp = *head;
	if (position == 0) {
		*head = temp->next;
		free(temp);
		return;
	}

	for (int i = 0; temp != NULL && i < position - 1; i++)
		temp = temp->next;
	if (temp == NULL || temp->next == NULL)
		return;
	Node* next = temp->next->next;
	free(temp->next);
	temp->next = next;
}
void insertAfter(Node* previous, std::string words)
{

	Node* new_node = new Node();
	new_node->word = words;
	new_node->next = previous->next;
	previous->next = new_node;
}
void destroy(Node** target) {
	Node* current = *target;
	Node* nextn = NULL;
	while (current != NULL) {
		nextn = current->next;
		free(current);
		current = nextn;
	}
	*target = NULL;
}

int main(int argc, char** argv)
{
	int printusage = 0;
	int	no_of_file;
	char** input_file_names;
	char* output_file_names;
	FILE* lzw_file;

	if (argc >= 3)
	{
		if (strcmp(argv[1], "-c") == 0)
		{
			/* compression */
			lzw_file = fopen(argv[2], "wb");

			/* write the file header */
			input_file_names = argv + 3;
			no_of_file = argc - 3;
			writefileheader(lzw_file, input_file_names, no_of_file);

			/* ADD CODES HERE */

			for (int index = 0; index < no_of_file; index++) {
				FILE* input = fopen(input_file_names[index], "rb");
				compress(input, lzw_file);
				fclose(input);
			}
			write_code(lzw_file, 0, 8);
			fclose(lzw_file);
		}
		else
			if (strcmp(argv[1], "-d") == 0)
			{
				/* decompress */
				lzw_file = fopen(argv[2], "rb");

				/* read the file header */
				no_of_file = 0;
				readfileheader(lzw_file, &output_file_names, &no_of_file);

				/* ADD CODES HERE */
				char* filename;
				filename = strtok(output_file_names, "\n");
				int i = 0;
				while (filename != NULL) {
					if (strlen(filename) != 0) {
						FILE* file = fopen(filename, "wb");
						decompress(lzw_file, file);
						fclose(file);
					}
					i++;
					if (i >= no_of_file) break;
					filename = strtok(NULL, "\n");
				}

				fclose(lzw_file);
				free(output_file_names);
			}
			else
				printusage = 1;
	}
	else
		printusage = 1;

	if (printusage)
		printf("Usage: %s -<c/d> <lzw filename> <list of files>\n", argv[0]);

	return 0;
}

/*****************************************************************
 *
 * writefileheader() -  write the lzw file header to support multiple files
 *
 ****************************************************************/
void writefileheader(FILE* lzw_file, char** input_file_names, int no_of_files)
{
	int i;
	/* write the file header */
	for (i = 0; i < no_of_files; i++)
	{
		fprintf(lzw_file, "%s\n", input_file_names[i]);

	}
	fputc('\n', lzw_file);

}

/*****************************************************************
 *
 * readfileheader() - read the fileheader from the lzw file
 *
 ****************************************************************/
void readfileheader(FILE* lzw_file, char** output_filenames, int* no_of_files)
{
	int noofchar;
	char c, lastc;

	noofchar = 0;
	lastc = 0;
	*no_of_files = 0;
	/* find where is the end of double newline */
	while ((c = fgetc(lzw_file)) != EOF)
	{
		noofchar++;
		if (c == '\n')
		{
			if (lastc == c)
				/* found double newline */
				break;
			(*no_of_files)++;
		}
		lastc = c;
	}

	if (c == EOF)
	{
		/* problem .... file may have corrupted*/
		*no_of_files = 0;
		return;

	}
	/* allocate memeory for the filenames */
	*output_filenames = (char*)malloc(sizeof(char) * noofchar);
	/* roll PW to start */
	fseek(lzw_file, 0, SEEK_SET);

	fread((*output_filenames), 1, (size_t)noofchar, lzw_file);

	return;
}

/*****************************************************************
 *
 * read_code() - reads a specific-size code from the code file
 *
 ****************************************************************/
unsigned int read_code(FILE* input, unsigned int code_size)
{
	unsigned int return_value;
	static int input_bit_count = 0;
	static unsigned long input_bit_buffer = 0L;

	/* The code file is treated as an input bit-stream. Each     */
	/*   character read is stored in input_bit_buffer, which     */
	/*   is 32-bit wide.                                         */

	/* input_bit_count stores the no. of bits left in the buffer */

	while (input_bit_count <= 24) {
		input_bit_buffer |= (unsigned long)getc(input) << (24 - input_bit_count);
		input_bit_count += 8;
	}

	return_value = input_bit_buffer >> (32 - code_size);
	input_bit_buffer <<= code_size;
	input_bit_count -= code_size;

	return(return_value);
}


/*****************************************************************
 *
 * write_code() - write a code (of specific length) to the file
 *
 ****************************************************************/
void write_code(FILE* output, unsigned int code, unsigned int code_size)
{
	static int output_bit_count = 0;
	static unsigned long output_bit_buffer = 0L;

	/* Each output code is first stored in output_bit_buffer,    */
	/*   which is 32-bit wide. Content in output_bit_buffer is   */
	/*   written to the output file in bytes.                    */

	/* output_bit_count stores the no. of bits left              */

	output_bit_buffer |= (unsigned long)code << (32 - code_size - output_bit_count);
	output_bit_count += code_size;

	while (output_bit_count >= 8) {
		putc(output_bit_buffer >> 24, output);
		output_bit_buffer <<= 8;
		output_bit_count -= 8;
	}


	/* only < 8 bits left in the buffer                          */

}

/*****************************************************************
 *
 * compress() - compress the source file and output the coded text
 *
 ****************************************************************/
void compress(FILE* input, FILE* output)
{

	unsigned int X = 0;
	int end = 256, index = 0;
	char C, N;
	std::string Cu = "";
	std::string m = "";
	Node* CodeDict = NULL;

	for (int i = 0; i <= 255; i++) {
		m = char(i);
		append(&CodeDict, m);
	}

	if ((C = fgetc(input)) == EOF)return;
	Cu = C;
	X = C;
	while ((N = fgetc(input)) != EOF) {
		index = search(CodeDict, Cu + N);

		if (index == end) {
			if (end >= 4096) {
				end = 256;
				destroy(&CodeDict);
				for (int i = 0; i <= 255; i++) {
					m = char(i);
					append(&CodeDict, m);
				}
			}
			write_code(output, X, CODE_SIZE);
			append(&CodeDict, Cu + N);

			end++;
			Cu = N;
			X = N;

		}
		else {
			Cu = Cu + N;
			X = index;

		}
	}
	write_code(output, X, CODE_SIZE);
	write_code(output, 4095, CODE_SIZE);
	delete CodeDict;
}


/*****************************************************************
 *
 * decompress() - decompress a compressed file to the orig. file
 *
 ****************************************************************/
void decompress(FILE* input, FILE* output)
{

	/* ADD CODES HERE */

	int end = 256;
	char C;
	std::string S = "", PW = "", StrDict[4096];
	for (int i = 0; i < 256; i++) {
		char n = i;
		StrDict[i] = n;
	}
	int NEW = 0;
	int OLD = read_code(input, CODE_SIZE);
	fputc(StrDict[OLD][0], output);
	while ((NEW = read_code(input, CODE_SIZE)) != 4095) {
		int search = 0;
		while (search < end) {
			if (StrDict[NEW] == StrDict[search])
			{
				S = StrDict[NEW]; break;
			}
			search++;
		}
		if (search == end) {
			S = StrDict[OLD] + C;
		}
		fputs(S.c_str(), output);
		C = S[0];

		if (end == 4096)end = 256;

		StrDict[end++] = StrDict[OLD] + C;
		OLD = NEW;
	}


}