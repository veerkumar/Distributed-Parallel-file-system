#include "commons.h"
#include "pfs.h"


#define ONEKB 1024
#define Filesize 150
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
    struct pfs_stat mystat;
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
  buf = (char*) malloc (sizeof(char)*Filesize);
  fseek(fid,4*ONEKB,SEEK_SET);
  int result = fread (buf,1,Filesize,fid);
  //cout<<buf;
  //std::this_thread::sleep_for(std::chrono::seconds(10));
  

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
  //Write the first 150 bytes of data from the input file onto pfs_file
  err_value = pfs_write(fdes, (void *)buf, Filesize, 0, &cache_hit);
  cout<<"Wrote %d bytes to the file\n", err_value;
  std::this_thread::sleep_for(std::chrono::seconds(20));
  
  /*err_value = pfs_read(fdes, (void *)buf, 50, 98, &cache_hit);
  printf("Read %d bytes of data from the file\n", err_value);
  cout<<buf;
  err_value = pfs_read(fdes, (void *)buf, 40, 60, &cache_hit);
  printf("Read %d bytes of data from the file\n", err_value);
  cout<<buf;*/
  //std::this_thread::sleep_for(std::chrono::seconds(120));
  pfs_fstat(fdes, &mystat);
  printf("File Metadata:\n");
  printf("Time of creation: %s\n", ctime(&(mystat.pst_ctime)));
  printf("Time of last modification: %s\n", ctime(&(mystat.pst_mtime)));
  printf("File Size: %d\n", mystat.pst_size);

  pfs_close(fdes);
  pfs_delete("pfs_file1");

  free(buf); 
  return 0;
}
