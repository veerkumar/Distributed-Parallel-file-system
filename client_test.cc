#include "commons.h"
#include "pfs.h"


#define ONEKB 1024

int main(int argc, char *argv[])
{
  int ifdes;
  uint32_t fdes;
  int err_value;
  char input_fname[20];
  char *buf;
  //char *readbuf;
  int cache_hit;
  ssize_t nread;
  string line;
  
  // Initialize the client
  initialize(argc, argv);

  cout<<"\n All initialization is done";
 
  // the command line arguments include an input filename
  if (argc < 2)
    {
      cout<<"usage: a.out <input filename>\n";
      exit(0);
    }
  strcpy(input_fname, argv[1]);

  ifstream file (input_fname);

  //buf = new char[4*ONEKB];
  
  FILE * fid=fopen(input_fname,"r");
  buf = (char*) malloc (sizeof(char)*4*ONEKB);
  int result = fread (buf,1,4*ONEKB,fid);
  cout<<buf;
  std::this_thread::sleep_for(std::chrono::seconds(10));
  

  // create a file only once, say at client 1 
  err_value = pfs_create("pfs_file1", 3);
  if(err_value < 0)
    {
      cout<<"Unable to create a file\n";
      exit(0);
    }

  // All the clients open the file 
  fdes = pfs_open("pfs_file1", 'w');
  if(fdes < 0)
    {
      cout<<"Error opening file\n";
      exit(0);
    }

  //At Client 1
  //Write the first 200 bytes of data from the input file onto pfs_file
  err_value = pfs_write(fdes, (void *)buf, 4*ONEKB, 0, &cache_hit);
  cout<<"Wrote %d bytes to the file\n", err_value;
  
  err_value = pfs_read(fdes, (void *)buf, 2*ONEKB, ONEKB, &cache_hit);
  printf("Read %d bytes of data from the file\n", err_value);
  std::this_thread::sleep_for(std::chrono::seconds(120));
  
  //pfs_close(fdes);

  free(buf); 
  return 0;
}
