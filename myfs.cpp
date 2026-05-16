#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <cstdint>    // for int types like uint32_t
#include <cstring>    // for memcpy()
#include <typeinfo>   // for reinterpret_cast

const char *MyFs::MYFS_MAGIC = "TBFS";
const char *BLOCK_SIZE = "256";
const char *FREE_BLOCK = "0";
const char *AMOUNT_OF_BLOCKS = "4096";  // 1MB / 256 (size of single block) = 4096 blocks
const char *OCCUPIED_BLOCK = "1";
const char *TYPE_FILE = "1";
// for calculating
const int INT_FREE_BLOCK = 0;
const int INT_BLOCK_SIZE = 256;
const int INT_AMOUNT_OF_BLOCKS = 4096;
const int START_INODE_TABLE = 5;
const int END_INODE_TABLE = 2053;
const int START_BLOCK_TABLE = 256;
const int END_BLOCK_TABLE = START_BLOCK_TABLE + 4*INT_BLOCK_SIZE;  // my all block table is blocks1-4

/* block 0 - super block
blocks 1-4 - block table
block 5-2053 - inode table
block 2053 - root folder
block 2054-4096 - our file system
all the file system is 1 mb */


// global
// this struct will help us with the inode
struct NodeStruct {
	std::string filename;   // max filename can be 10, each char is 1 byte so all filename is 10 bytes
    int typeFile;  // 4 bytes
    int sizeFile;  // 4 bytes
    int ptrArray[MAX_BLOCKS_TO_FILE]; // 4 * 6 = 24 bytes 
};
const int STRUCT_SIZE = 10 + 4 + 4 + 6 * 4;
const int FILENAME_SIZE = 10;
const int TYPE_FILE_SIZE = 4;
const int SIZE_FILE_SIZE = 4;
const int PTR_ARRAY_SIZE = 24;
NodeStruct myInode[INT_BLOCK_SIZE] = {};   // init all to 0

int blockTable[2046] = {};

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
}

void MyFs::format() {

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);
	// TODO: put your format code here

	/*Block size: the file system needs to know the size of each block that the file system uses
	In it, because this number is necessary for the calculations of the various locations

	Number of blocks: The fs must know how many blocks are in the file system
	to be able to save files correctly. If the fs is not aware that they exist,
	for example, only 254 blocks, it may ask the physical device to store information in the 254th block*/
	blkdevsim->write(sizeof(header), sizeof(BLOCK_SIZE), BLOCK_SIZE);  // writing the size of each block int the superblock
	blkdevsim->write((sizeof(header) + sizeof(BLOCK_SIZE)), sizeof(AMOUNT_OF_BLOCKS), AMOUNT_OF_BLOCKS);  // writing the max blocks I can hold in my fs
	
	// the block table
	void* ptr2 = blockTable;
	memset(ptr2, '0', INT_BLOCK_SIZE*4);
	blkdevsim->write(INT_BLOCK_SIZE, INT_BLOCK_SIZE*4, (char*)ptr2);

	// if I will do bonus directory i will need to initillize the root folder, for each folder will be pointer
	// to the folder and to the root folder 
}

void MyFs::create_file(std::string path_str, bool directory) {
	size_t pos = path_str.find_last_of('/'); // find last occurrence of '/'
    std::string filename = path_str.substr(pos + 1); // extract substring after '/'
	if(isFilenameAlreadyExist(filename))
		throw std::runtime_error("Filename already exist! try different one");

	bool isThereFreePlace = false;
	for(int i = START_BLOCK_TABLE; i < END_BLOCK_TABLE; i++)  // looking for an empty block in the blocktable
	{
		char* ans = new char[2];
		blkdevsim->read(i, 1, ans);
		ans[1] = '\0'; // set second byte to null terminator
		if (strcmp(ans, FREE_BLOCK) == 0)  // we found an empty block
		{
			isThereFreePlace = true;
			int nodeNumber = i - INT_BLOCK_SIZE;
			
			// write the name of the file
			int value = i - INT_BLOCK_SIZE;
			std::string value_str = std::to_string(value);
			blkdevsim->write((START_INODE_TABLE*INT_BLOCK_SIZE)+(nodeNumber*INT_BLOCK_SIZE), FILENAME_SIZE, filename.c_str());

			// 1 - file, 2 - directory, 0 - delete
			blkdevsim->write((START_INODE_TABLE*INT_BLOCK_SIZE)+(nodeNumber*INT_BLOCK_SIZE)+FILENAME_SIZE+value_str.size(), TYPE_FILE_SIZE, TYPE_FILE);
			delete[] ans;
			break; // no need to continue, we already found
		}
		delete[] ans;
	}
	if(!isThereFreePlace) throw std::runtime_error("File system is full!");
}


/*extract content of the file*/
std::string MyFs::get_content(std::string path_str) 
{
	size_t pos = path_str.find_last_of('/'); // find last occurrence of '/'
    std::string filename = path_str.substr(pos + 1); // extract substring after '/
	
	// check if the file is exist
	if(!isFilenameAlreadyExist(filename))
		throw std::runtime_error(filename + " not exist! use touch first");
	
	int size = extractSizeFileFromName(filename);  // size of the file
	
	if(size == 99999) return "";
	int fullBlocks = size / INT_BLOCK_SIZE;
	int restBlockSize =  size - (INT_BLOCK_SIZE * fullBlocks);
	std::string content = "";
	int b=0;

	std::array<int, MAX_BLOCKS_TO_FILE> blocks = extractBlocksOfFile(filename); // array that contains the number of the block
	
	int startData = (END_INODE_TABLE+1)*INT_BLOCK_SIZE; // start data in our fs
	for(int i = 0; i < MAX_BLOCKS_TO_FILE; i++)  // run on the blocks and building the content string
	{
		if(blocks[b] == 999) break;  // end of blocks
		char* ans;
		ans = fullBlocks == 0 ? new char[restBlockSize] : new char[INT_BLOCK_SIZE];
		
		int pos = startData + INT_BLOCK_SIZE*blocks[b];
		blkdevsim->read(pos, fullBlocks == 0 ? restBlockSize : INT_BLOCK_SIZE, ans);
		content += ans;

		fullBlocks--;
		delete[] ans;
		b++;
	}
	return content;
}

void MyFs::set_content(std::string path_str, std::string content) 
{
	size_t pos = path_str.find_last_of('/'); // find last occurrence of '/'
    std::string filename = path_str.substr(pos + 1); // extract substring after '/'
	
	if(!isFilenameAlreadyExist(filename))
		throw std::runtime_error(filename + " not exist! use touch first");

	int sizeContent = content.length();
	std::array<int, MAX_BLOCKS_TO_FILE> blocks = extractBlocksOfFile(filename);

	int fullBlocks = sizeContent / INT_BLOCK_SIZE;  
	int restBlockSize =  sizeContent - (INT_BLOCK_SIZE * fullBlocks);
	if(sizeContent <= INT_BLOCK_SIZE)
		restBlockSize = sizeContent;

	if(fullBlocks > MAX_BLOCKS_TO_FILE)
		throw std::runtime_error("your content is too long, try shorter one");

	int index = 0;
	// searching an emptys blocks	
	for(index = 0; index < MAX_BLOCKS_TO_FILE; index++)
	{
		if(blocks[index] == 999) break;
	}
	
	if(index == MAX_BLOCKS_TO_FILE - 1 && blocks[index] == 999)
		throw std::runtime_error(filename + " is full! try another file");
	
	int count = 0;
	int stop = fullBlocks;
	stop += restBlockSize > 0 ? 1 : 0;
	// update block table
	for (int i = START_BLOCK_TABLE; i < END_BLOCK_TABLE && count != stop; i++) {
    	char* ans = new char[2];  // max chars in filename is 10
    	blkdevsim->read(i, 1, ans);
    	ans[1] = '\0';
		const char* occupied = "0";
    	if (ans[0] == occupied[0]) {
        	blkdevsim->write(i, 1, OCCUPIED_BLOCK);
        	blocks[index + count] = i - START_BLOCK_TABLE;
        	count++;
    	}
    	delete[] ans;
	}

	// update ptrArray in inode
	for(int i = 0; i < MAX_BLOCKS_TO_FILE && blocks[i] != 999; i++)  // need to fix
	{
		int value = blocks[i];
		std::string value_str = std::to_string(value);
		int ptrArraySize = (START_INODE_TABLE*INT_BLOCK_SIZE)+(blocks[0]*INT_BLOCK_SIZE)+(FILENAME_SIZE+TYPE_FILE_SIZE+SIZE_FILE_SIZE+i*4);
		blkdevsim->write(ptrArraySize, PTR_ARRAY_SIZE/MAX_BLOCKS_TO_FILE, value_str.c_str());
	}
	// searching free blocks in the block table and change them to occupied
	if(fullBlocks == 0)  // no need fragmentation
	{
		// update the data
		if(restBlockSize - content.length() >= 0)  // there is no need for extra block
		{
			int start = (END_INODE_TABLE+1)*INT_BLOCK_SIZE+(blocks[index]*INT_BLOCK_SIZE);
			blkdevsim->write(start, sizeContent, content.c_str());
		}
		// update in blocktable
		blkdevsim->write((INT_BLOCK_SIZE+blocks[index]), 1, OCCUPIED_BLOCK);
		// update the inode (size file)
		std::string sizeString = std::to_string(sizeContent);
		const char* sizeChar = sizeString.c_str();
		int start = (INT_BLOCK_SIZE*START_INODE_TABLE)+(INT_BLOCK_SIZE*blocks[index])+FILENAME_SIZE+TYPE_FILE_SIZE;
		blkdevsim->write(start, SIZE_FILE_SIZE, sizeChar);
		return;
	}
	// fragmentaion
	std::array<std::string, MAX_BLOCKS_TO_FILE> splitContent = splitString(content, fullBlocks, INT_BLOCK_SIZE);
	// update data
	int lastContent = 0;
	int i=0;
	for(i = 0; i < fullBlocks && fullBlocks > 0; i++)
	{
		if(splitContent[i] == "999") break;
		int start = (END_INODE_TABLE+1)*INT_BLOCK_SIZE+(blocks[i]*INT_BLOCK_SIZE);
		blkdevsim->write(start, INT_BLOCK_SIZE, splitContent[i].c_str());
		lastContent++;
	}
	// update data in the rest block 
	blkdevsim->write((END_INODE_TABLE+1)*INT_BLOCK_SIZE+(blocks[i]*INT_BLOCK_SIZE), restBlockSize, splitContent[i].c_str());
	// update the inode (size file)
	std::string sizeString = std::to_string(sizeContent);
	const char* sizeChar = sizeString.c_str();
	blkdevsim->write((INT_BLOCK_SIZE*START_INODE_TABLE)+(INT_BLOCK_SIZE*blocks[0])+FILENAME_SIZE+TYPE_FILE_SIZE, SIZE_FILE_SIZE, sizeChar);
}

MyFs::dir_list MyFs::list_dir(std::string path_str) {
	dir_list ans;
	throw std::runtime_error("not implemented");
	return ans;
}

bool MyFs::isFilenameAlreadyExist(std::string filename)
{
    for(int i = START_INODE_TABLE; i < END_INODE_TABLE; i++)  // looking for an empty block in the blocktable
	{
		char* ans = new char[filename.size()];  // max chars in filename is 10
		ans[filename.size()] = '\0';
		blkdevsim->read(i*INT_BLOCK_SIZE, filename.size(), ans);
		if (std::string(ans) == filename)  // we found the same filename
			return true;
		delete[] ans;
	}
	return false;
}

/*Creates an array of blocks of type int where each number represents the block number of the file,
it runs on the inode table until it finds by name and adds the block numbers*/
std::array<int, MAX_BLOCKS_TO_FILE> MyFs::extractBlocksOfFile(std::string filename)
{
    std::array<int, MAX_BLOCKS_TO_FILE> blocks = {999, 999, 999, 999, 999, 999};
	if(!isFilenameAlreadyExist(filename))
		throw std::runtime_error("Filename not exist! use touch first.");
	// run on the inode and searching the filename
	for(int i = START_INODE_TABLE; i < END_INODE_TABLE; i++)
	{
		char* ansPtrArray = new char[2];
		char* ansFilename = new char[filename.length()];  // max chars in filename is 10
		blkdevsim->read(i*INT_BLOCK_SIZE, filename.length(), ansFilename);
		ansPtrArray[2] = '\0';
		if (std::string(ansFilename) == filename)
		{
			// found the filename, adding blocks to the array
			int index = 0;
			for(int j = 0; j < MAX_BLOCKS_TO_FILE*4; j+=4) // j+=4 every int is 4 bytes
			{
				int positionArrayPtr = i*INT_BLOCK_SIZE+FILENAME_SIZE+TYPE_FILE_SIZE+SIZE_FILE_SIZE+j;
				blkdevsim->read(positionArrayPtr, 4, ansPtrArray);
				if((strcmp(ansPtrArray, "0") == 0 && j != 0) || (ansPtrArray == 0 && j == 0)) // only first block can be 0
					continue;
				if(ansPtrArray[0] != '\0')
					blocks[index] = std::stoi(ansPtrArray);
				index++;
			}	
			break;
		}
		delete[] ansPtrArray;
		delete[] ansFilename;
	}
	return blocks;
}

int MyFs::extractSizeFileFromName(std::string filename)
{
    for(int i = START_INODE_TABLE; i < END_INODE_TABLE; i++)  // looking for an empty block in the blocktable
	{
		char* ans = new char[filename.size()+1];  // max chars in filename is 10
		ans[filename.length()] = '\0';
		blkdevsim->read(i*INT_BLOCK_SIZE, filename.length(), ans);
		if ((strcmp(ans, filename.c_str()) == 0) )  // we found the same filename
		{
			char* ans2 = new char[SIZE_FILE_SIZE+1];
			int start = i*INT_BLOCK_SIZE+FILENAME_SIZE+TYPE_FILE_SIZE;
			blkdevsim->read(start, SIZE_FILE_SIZE, ans2);
			int result = 0;
			for (int i = 0; i < SIZE_FILE_SIZE; i++) {
    			if (std::isdigit(ans2[i])) {
        			result = result * 10 + (ans2[i] - '0');
 		   		}
			}			
			delete[] ans;
			delete[] ans2;
			return result;
		}
		delete[] ans;
	}
	return 99999;
}

std::array<std::string, MAX_BLOCKS_TO_FILE> MyFs::splitString(std::string str, int limit, int chunk_size)
{
    std::array<std::string, MAX_BLOCKS_TO_FILE> result = {"999", "999", "999", "999", "999", "999"};
    int pos = 0;
    for (int i = 0; i < limit; i++) {
        result[i] = str.substr(pos, chunk_size);
        pos += chunk_size;
    }
    result[limit] = str.substr(pos);
    return result;
}
