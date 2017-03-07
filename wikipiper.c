
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * 
 * wikipiper.c 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * 
 * intended usage: 
 * 
 *   >> gcc wikipiper.c -o wikipiper
 *   >> curl <url> | ./wikipiper(.exe on windows)
 * 
 * where url might be
 * 
 *   https://wikimedia.org/api/rest_v1/metrics/pageviews/per-article/en.wikipedia/all-access/all-agents/Albert_Einstein/daily/2010010100/2016123100
 * 
 * We expect input of the form
 * 
 *   {"type":  ... } for an error
 *   {"items":[...]} for a valid list, wherein each element is
 *      {"project":"en.wikipedia",
 *			"article":"Albert_Einstein",
 *			"granularity":"daily",
 *			"timestamp":"2016123000",
 *			"access":"all-access",
 *			"agent":"all-agents",
 *			"views":14770}
 * 
 * Our goal is to strip out timestamp and views, also parsing the timestamp into separate year-month-day fields. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *
 * NOTES: 
 * 
 * 1. Work TBD: need to write code to check completeness of the line read given a certain buffer size, and store/read 
 *    accordingly. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 */

// size of buffer for reading through 
#define BUFFER_SIZE 1000000

int main( int argc , char * argv[] )
{
	unsigned int i;
	char *c , *d , *k , *v , line[BUFFER_SIZE] , tag[1024] , key[1024] , val[1024];
	unsigned int r = 0;
	char ofn[1024];
	
	long int year, month, day, views;
	FILE * fp;
	
	strcpy( ofn , "out" );
	if( argc > 1 ) { strcpy( ofn , argv[1] ); }
	strcat( ofn , ".csv" );
	printf( "WIKIPIPER: writing to... %s\n" , ofn );
	fp = fopen( ofn , "w" );
	if( fp == NULL ) { printf("WIKIPIPER: couldn't open output file.\n"); return EXIT_FAILURE; } 
	
    while( fgets( line , sizeof(line) , stdin ) ) {
		
		if( r == 0 ) { // check first read
			if( sscanf( line , "{\"%1024[^\"]" , tag ) > 0 ) {
				if( strcmp( tag , "items" ) != 0 ) {
					printf( "WIKIPIPER: error string detected (%s)\n" , line );
					return 1;
				}
			} else {
				printf( "WIKIPIPER: uninterpretable string (%s)\n" , line );
				return 1;
			}
				
		}
		
		// if we're here, we are reading an items list... we know 
		// the value passed in starts with 
		//
		//    {"items"
		//
		// so we hope next two match... 
		if( line[8] == ':' && line[9] == '[' ) {
			
			i = 10; //starting here, we expect sets of "{ ... }" as above
			c = line + 10;
			while( c[0] == '{' ) { 
				// read this tag (or try)
				if( sscanf( c , "{%1024[^}]}" , tag ) > 0 ) {
					// printf( "WIKIPIPER: read tag: %s\n" , tag );
					// read the key-value pairs in tag
					d = tag;
					while( sscanf( d , "%1024[^:,}]:%1024[^,}]" , key , val ) >= 2 ) {
						// increment the pointer (before we modify any data and string lengths)
						d += strlen(key) + strlen(val) + 1;
						// ok, now we can parse keys and values... 
						k = key + strlen(key)-1; while( k[0] == '"' ) { k[0] = '\0'; k--; }
						k = key; while( k[0] == '"' ) { k++; }
						v = val + strlen(val)-1; while( v[0] == '"' ) { v[0] = '\0'; v--; }
						v = val; while( v[0] == '"' ) { v++; }
						// printf( "WIKIPIPER:   read key/value pair ( %s , %s )\n" , k , v );
						switch( k[0] ) {
							case 't': // timestamp
								v[strlen(v)-1] = '\0'; // remove trailing zeros
								v[strlen(v)-1] = '\0'; // second one... 
								day = strtol( v + strlen(v) - 2 , NULL , 10 );
								v[strlen(v)-1] = '\0'; // remove trailing zeros
								v[strlen(v)-1] = '\0'; // second one... 
								month = strtol( v + strlen(v) - 2 , NULL , 10 );
								v[strlen(v)-1] = '\0'; // remove trailing zeros
								v[strlen(v)-1] = '\0'; // second one... 
								year = strtol( v , NULL , 10 );
								break;
							case 'v': // views
								views = strtol( v , NULL , 10 );
								break;
							default: break;
						}
						// skip past comma, if it exists
						if( d[0] == ',' ) { d++; }
					}
					fprintf( fp , "%li-%li-%li,%li\n" , year , month , day , views );
					
					c += strlen(tag) + 2;
				}
				if( c[0] == ',' ) { c++; }
			}
			
		} else {
			printf( "WIKIPIPER: characters following {%citems%c (%c%c)don't match \n" , '"' , '"' , line[8] , line[9] );
			return 1;
		}
		
		r++;
		
	}
	
	fclose( fp );
}