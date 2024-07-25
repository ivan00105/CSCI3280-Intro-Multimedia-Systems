/*
* CSCI3280 Introduction to Multimedia Systems *
* --- Declaration --- *
* I declare that the assignment here submitted is original except for source
* material explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/ *
* Assignment 2
* Name : Wong Shing Lok
* Student ID : 1155156680
* Email Addr : 1155156680@link.cuhk.edu.hk
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <vector> // remove this if you are writing a C program.

#define CODE_SIZE  12 // you can use CODE_SIZE=16 if you want your compressed files to be 
                      // human-readable during debugging (it is required to be 12 for submission)
#define TRUE 1
#define FALSE 0

using namespace std;

struct Entry {
    int prev_char;
    int current_char;
};

Entry table[4096];
int next_entry;

/* function prototypes */
unsigned int read_code(FILE*, unsigned int); 
void write_code(FILE*, unsigned int, unsigned int); 
void writefileheader(FILE *,char**,int);
void readfileheader(FILE *,char**,int *);
void compress(FILE*, FILE*);
void decompress(FILE*, FILE*);
void write_code(FILE*, int );
void initialize_table();

void initialize_table() {
	table[4095] = { -1, EOF };
    for (next_entry = 0; next_entry < 256; ++next_entry)
		table[next_entry] = { -1, next_entry };
}


int main(int argc, char **argv)
{
    int printusage = 0;
    int	no_of_file;
    char **input_file_names;    
	char *output_file_names;
    FILE *lzw_file;

    if (argc >= 3)
    {
		if ( strcmp(argv[1],"-c") == 0)
		{		
			/* compression */
			lzw_file = fopen(argv[2] ,"wb");
        
			/* write the file header */
			input_file_names = argv + 3;
			no_of_file = argc - 3;
			writefileheader(lzw_file,input_file_names,no_of_file);

			/* ADD CODES HERE */
            initialize_table();
			for(int i = 0; i < no_of_file; ++i){
				FILE* input_file = fopen(input_file_names[i], "rb");
				compress(input_file, lzw_file);
				fclose(input_file);
			}
			//flush the remaining 8 bits
			write_code(lzw_file,0,8);
			fclose(lzw_file);        	
		} else
		if ( strcmp(argv[1],"-d") == 0)
		{	
			/* decompress */
			lzw_file = fopen(argv[2] ,"rb");
			
			/* read the file header */
			no_of_file = 0;			
			readfileheader(lzw_file,&output_file_names,&no_of_file);
			
			/* ADD CODES HERE */
            initialize_table();
            char * files = strtok(output_file_names, "\n");
			for(int i = 0; i < no_of_file; ++i){
				FILE* output_file = fopen(files, "wb");
				decompress(lzw_file, output_file);
				fclose(output_file);
				files = strtok(NULL,"\n");
			}
			fclose(lzw_file);		
			free(output_file_names);
		}else
			printusage = 1;
    }else
		printusage = 1;

	if (printusage)
		printf("Usage: %s -<c/d> <lzw filename> <list of files>\n",argv[0]);
 	
	return 0;
}

/*****************************************************************
 *
 * writefileheader() -  write the lzw file header to support multiple files
 * arguments:
 * 	lzw_file - the output file during compression
 * 	input_file_names - array of char arrays, containing the input filenames
 *  no_of_files - number of files in the input
 *
 ****************************************************************/
void writefileheader(FILE *lzw_file,char** input_file_names,int no_of_files)
{
	int i;
	/* write the file header */
	for ( i = 0 ; i < no_of_files; i++) 
	{
		fprintf(lzw_file,"%s\n",input_file_names[i]);	
			
	}
	fputc('\n',lzw_file);

}

/*****************************************************************
 *
 * readfileheader() - read the fileheader from the lzw file
 * arguments:
 * 	lzw_file - the input file during decompression
 * 	output_file_names - a char array passed by pointer, will be 
 * 		filled with chars containing all file names (you need to
 *  	split the filenames manually by "\n" yourself)
 * 	no_of_files - number of files in the file header, passed by
 * 		pointer
 * behaviour: this function leave the file pointer at the first code of
 * 		the code sequence. You can move the file pointer to the start
 * 		of code sequence using this function if you messed up with the
 * 		file pointer location.
 *
 ****************************************************************/
void readfileheader(FILE *lzw_file,char** output_filenames,int * no_of_files)
{
	int noofchar;
	char c,lastc;

	noofchar = 0;
	lastc = 0;
	*no_of_files=0;
	/* find where is the end of double newline */
	while((c = fgetc(lzw_file)) != EOF)
	{
		noofchar++;
		if (c =='\n')
		{
			if (lastc == c )
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
	*output_filenames = (char *) malloc(sizeof(char)*noofchar);
	/* roll back to start */
	fseek(lzw_file,0,SEEK_SET);

	fread((*output_filenames),1,(size_t)noofchar,lzw_file);
	
	return;
}

/*****************************************************************
 *
 * read_code() - reads a specific-size code from the code file
 * arguments:
 * 	input: the file pointer of input file. Make sure you file pointer
 * 		is at the start of the code sequence before you call this
 * 		function for the first time.
 * 	code_size: the length of code in bits. It should be 12 for reading
 * 		12-bit code.
 * 	behaviour: reads in one code each time. Will return the maximum
 * 		value if the data left in the file is less than the length
 * 		indicated by code_size.
 *
 ****************************************************************/
unsigned int read_code(FILE *input, unsigned int code_size)
{
    unsigned int return_value;
    static int input_bit_count = 0;
    static unsigned long input_bit_buffer = 0L;

    /* The code file is treated as an input bit-stream. Each     */
    /*   character read is stored in input_bit_buffer, which     */
    /*   is 32-bit wide.                                         */

    /* input_bit_count stores the no. of bits left in the buffer */

    while (input_bit_count <= 24) {
        input_bit_buffer |= (unsigned long) (unsigned char) getc(input) << (24-input_bit_count);
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
 * arguments:
 * 	output: the file pointer of the output file.
 * 	code: an unsigned code value you want to write.
 * 	code_size: the length of code in bits. It should be 12 for reading
 * 		12-bit code.
 * behaviour: only flushes the code to the file when there is more than
 * 		8-bits of data in its buffer. So, to make sure all important data are written
 * 		to the compressed file, write at least 8 extra bits using this function
 * 		after you write the last code. (Writing a <4095> after the last code
 * 		of the last file will do the job)
 *
 ****************************************************************/
void write_code(FILE *output, unsigned int code, unsigned int code_size)
{
    static int output_bit_count = 0;
    static unsigned long output_bit_buffer = 0L;

    /* Each output code is first stored in output_bit_buffer,    */
    /*   which is 32-bit wide. Content in output_bit_buffer is   */
    /*   written to the output file in bytes.                    */

    /* output_bit_count stores the no. of bits left              */    

    output_bit_buffer |= (unsigned long) code << (32-code_size-output_bit_count);
    output_bit_count += code_size;

    while (output_bit_count >= 8) {
        putc(output_bit_buffer >> 24, output);
        output_bit_buffer <<= 8;
        output_bit_count -= 8;
    }


    /* only < 8 bits left in the buffer                          */    

}

void write_code(FILE* output, int code) {
	vector<int> stack;
	while (code != -1) {
		stack.push_back(code);
		code = table[code].prev_char;
	}

	for (auto i = stack.rbegin(); i != stack.rend(); ++i) {
		fputc(table[*i].current_char, output);
	}
}

/*****************************************************************
 *
 * compress() - compress the source file and output the coded text
 * hints: write this function yourself according to the LZW algorithm.
 * 		You are free to change the arguments of this function.
 *
 ****************************************************************/
void compress(FILE *input, FILE *output)
{
	/* ADD CODES HERE */
    int prev, cur;
	// read the first character from input file
    prev = fgetc(input);
	// return if file is empty
    if (prev == EOF) return;

	// loop until end of input file
    while ((cur = fgetc(input))!= EOF){
        // find if the char is in table
        int char_index = -1;
		// search the table for the current combination of previous and current char
		for (int i = 0; i < next_entry; ++i) {
			if (table[i].prev_char == prev && table[i].current_char == cur) {
				char_index = i; // current index of entry e
				break;
			}
		}
		// if not found, output the code for the previous char and add them to the table
		if (char_index == -1) {
			write_code(output, prev, CODE_SIZE);
			table[next_entry++] = { prev, cur };
		}
		// set the previous char to the current char if not found. Otherwise, set the previous char to the found pair in table.
		prev = char_index != -1 ? char_index : cur;
		
		// set the next entry to index 256 if running out of entries
		if (next_entry == 4096) {
			next_entry = 256;
			table[4095] = { -1, EOF };
		}

    }
	// output the remaining chars and EOF
    write_code(output, prev, CODE_SIZE);
    write_code(output, 4095, CODE_SIZE);
}


/*****************************************************************
 *
 * decompress() - decompress a compressed file to the orig. file
 * hints: write this function yourself according to the LZW algorithm.
 * 		You are free to change the arguments of this function.
 *
 ****************************************************************/
void decompress(FILE *input, FILE *output)
{
	/* ADD CODES HERE */
	int code, temp;
	int cur_code, prev_code;

	// read and write the first code
	cur_code = read_code(input, CODE_SIZE);
	write_code(output, cur_code);

	// loop code until 4095 (EOF)
	while ((code = read_code(input, CODE_SIZE)) != 4095) {
		prev_code = cur_code;
		cur_code = code;
		// find the index of the current code in the table
		// if not, use the previous code instead.
		int code_index = cur_code < next_entry ? cur_code : prev_code;
		while (table[code_index].prev_char != -1) {
			code_index = table[code_index].prev_char; // code_index points to the index of the last character
		}
		// write the current code to output file and update temp
		write_code(output, cur_code < next_entry ? cur_code : prev_code);

		// if current code >= next entry, write the temp to output
		temp = table[code_index].current_char;
		if(cur_code >= next_entry) fputc(temp, output);

		// add new entry to the table
		table[next_entry++] = { prev_code, temp };
		
		// set the next entry to index 256 if running out of entries
		if (next_entry == 4096) {
			next_entry = 256;
			table[4095] = { -1, EOF };
		}
	}
}
