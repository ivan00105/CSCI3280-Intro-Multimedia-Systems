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
#include <iostream>
#include <unordered_map>
#include <string>

#define CODE_SIZE  12 // you can use CODE_SIZE=16 if you want your compressed files to be 
                      // human-readable during debugging (it is required to be 12 for submission)
#define TRUE 1
#define FALSE 0

using namespace std;

int next_entry;
unordered_map<string, int> dict_compress;
unordered_map<int, string> dict_decompress;

/* function prototypes */
unsigned int read_code(FILE*, unsigned int); 
void write_code(FILE*, unsigned int, unsigned int); 
void writefileheader(FILE *,char**,int);
void readfileheader(FILE *,char**,int *);
void compress(FILE*, FILE*);
void decompress(FILE*, FILE*);
void initialize_dict(char);
void RLEcompress(FILE* input, FILE* output);
void RLEdecompress(FILE* input, FILE* output);




void initialize_dict(char c) {
	next_entry = 256;
	if(c=='c'){
		for (int i = 0; i < next_entry; i++) {
			dict_compress[string(1, (char)i)] = i;
		}
	}
	else {
		for (int i = 0; i < next_entry; i++) {
			dict_decompress[i] = string(1, (char)i);
		}
	}
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
		if (strcmp(argv[1], "-RLEC") == 0) {
			/* compression */
			lzw_file = fopen(argv[2], "wb");

			/* write the file header */
			input_file_names = argv + 3;
			no_of_file = argc - 3;
			writefileheader(lzw_file, input_file_names, no_of_file);

			for (int i = 0; i < no_of_file; ++i) {
				FILE* input_file = fopen(input_file_names[i], "rb");
				RLEcompress(input_file, lzw_file);
				fclose(input_file);
			}
			fclose(lzw_file);
		}else
		if (strcmp(argv[1], "-RLED") == 0) {
			/* decompress */
			lzw_file = fopen(argv[2], "rb");

			/* read the file header */
			no_of_file = 0;
			readfileheader(lzw_file, &output_file_names, &no_of_file);

			char* files = strtok(output_file_names, "\n");
			for (int i = 0; i < no_of_file; ++i) {
				FILE* output_file = fopen(files, "wb");
				RLEdecompress(lzw_file, output_file);
				fclose(output_file);
				files = strtok(NULL, "\n");
			}
			fclose(lzw_file);
			free(output_file_names);
		}
		else
		if ( strcmp(argv[1],"-c") == 0)
		{		
			/* compression */
			lzw_file = fopen(argv[2] ,"wb");
        
			/* write the file header */
			input_file_names = argv + 3;
			no_of_file = argc - 3;
			writefileheader(lzw_file,input_file_names,no_of_file);

			/* ADD CODES HERE */
            initialize_dict('c');
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
            initialize_dict('d');
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


/*****************************************************************
 *
 * compress() - compress the source file and output the coded text
 * hints: write this function yourself according to the LZW algorithm.
 * 		You are free to change the arguments of this function.
 *
 ****************************************************************/
void compress(FILE *input, FILE *output)
{
	int cur, prev;
	prev = fgetc(input);
	if (prev == EOF) return;
	//first char
	string str = string(1, (char)prev);
	while ((cur = fgetc(input)) != EOF) {
		string str_plus_cur = str + (char)cur;
		
		// check if str+cur is in the dictionary
		if (dict_compress.count(str_plus_cur)) 
			str = str_plus_cur;

		else {
			// output the code for str
			write_code(output, dict_compress[str], CODE_SIZE);
			// add str+cur to the dictionary
			if (next_entry < 4096) {
				dict_compress[str_plus_cur] = next_entry++;
			}
			// set str to cur
			str = string(1, (char)cur);
		}
		// reset dictionary if it is full
		if (next_entry == 4096) {
			dict_compress.clear();
			initialize_dict('c');
		}
	}
	// output the code for the last str
	write_code(output, dict_compress[str], CODE_SIZE);
	// output the end-of-file marker
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
	int prev_code = -1, cur_code;
	string str;
	while ((cur_code = read_code(input, CODE_SIZE)) != 4095) {
		// decode the current code
		string s;
		// output the string if the code is in dictionary
		if (dict_decompress.count(cur_code)) {
			s = dict_decompress[cur_code];
		}
		// current string == previous string + its first character
		else if (cur_code == next_entry) {
			s = str + str[0];
		}
		fwrite(s.data(), 1, s.length(), output);
		// new entry = concatenation of the previous with the first char of the current str
		if (prev_code != -1 && next_entry < 4096) {
			dict_decompress[next_entry++] = dict_decompress[prev_code] + s[0];
		}
		// update previous code and str
		prev_code = cur_code;
		str = s;
		// start over if the dictionary is full
		if (next_entry == 4096) {
			dict_decompress.clear();
			initialize_dict('d');
		}
	}
}


void RLEcompress(FILE* input, FILE* output) {
	int prev = fgetc(input);
	if (prev == EOF) return;
	int count = 1;
	int cur;
	while ((cur = fgetc(input)) != EOF) {
		// max count = 4095 - 1 (12 bit)
		if (cur == prev && count < 4094) 
			++count;
		
		else {
			write_code(output, prev, CODE_SIZE);
			write_code(output, count, CODE_SIZE);
			prev = cur;
			count = 1;
		}
	}
	write_code(output, prev, CODE_SIZE);
	write_code(output, count, CODE_SIZE);
	//EOF
	write_code(output, 4095, CODE_SIZE);
}

void RLEdecompress(FILE* input, FILE* output) {
	int cur;
	while ((cur = read_code(input, CODE_SIZE)) != 4095) {
		int code = cur;
		int count = read_code(input, CODE_SIZE);
		for (int i = 0; i < count; ++i) {
			fputc(code, output);
		}
	}
}

