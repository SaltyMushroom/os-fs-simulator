#ifndef __file_system__
#define __file_system__

#define BLOCKSIZE 512 //�����С512B
#define SIZE 4096000  //����һ��4MB���ļ��洢�ռ�
//һ��8000��
#define MAXOPEN_USER 50   //�û������ͬʱ�򿪵��ļ�����
#define MAXOPEN_SYSTEM 50 //ϵͳ�����ͬʱ�򿪵��ļ�����
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

//sizeof(FCB)=40, sizeof(INode)=40, total_size = 80, 80*256=20480=40*BLOCKSIZE
//BLOCK 1-40: FCB*224+INode*284
typedef struct FCB
{
	char filename[32];
	unsigned short inodeIndex; //inode�ţ��ļ��ţ�
} FCB;

typedef struct INode
{
	unsigned char attribute; //0: directory 1:file
	unsigned char status;	//״̬ 0��δռ�� 1����ռ��
	unsigned int length;	 //�ļ�����
	int directPointer[8];	//ֱ��ָ��8��
	int indirectPointer;	 //1�����ָ��1��
	int subIndirectPointer;  //2�����ָ��1��
} INode;

//sizeof(Directory) = BLOCKSIZE
typedef struct Directory
{
	unsigned int parentINodeIndex; //����Ŀ¼��inode��
	int files[127];				   //�����ļ���inode��
} Directory;

//sizeof(INodeBlock) = BLOCKSIZE
typedef struct INodeBlock
{
	int pointers[128]; //�������
} INodeBlock;

typedef struct USEROPEN
{								   //�û��򿪱�
	unsigned char openMode;		   //�򿪷�ʽ	0:r 1:w 2:rw
	unsigned char systemOpenEntry; //ϵͳ�ļ��򿪱����
	unsigned char status;		   //״̬
	unsigned short parentIndex;	//�������û��򿪱��λ��
} USEROPEN;

typedef struct SYSTEMOPEN
{ //ϵͳ�򿪱�
	char filename[32];
	unsigned char attribute;   //0: directory 1:file
	unsigned int length;	   //�ļ�����
	unsigned short inodeIndex; //inode��
	unsigned char status;	  //FCB��״̬
	unsigned int shareCount;   //�������Ŀ

	unsigned short parentIndex; //���ļ�Ŀ¼�ڴ򿪱��λ��
	unsigned short parentINode; //���ļ�Ŀ¼��INode ID
} SYSTEMOPEN;

typedef struct blockBit
{
	int val : 1;
} blockBit;

//==== ָ����� ====
void *FILEBLOCK_START; //�ļ��ռ���ʼλ��

//==== �������� ====
int totalFCB = 256;
int usedFCB = 0;
int freeFCB = 256;
int totalBlock = 8000;
int usedBlock = 0;
int freeBlock = 8000;

int currentDirectory = -1;		//��ǰĿ¼���ļ���
int currentDirectoryIndex = -1; //��ǰĿ¼���û��ļ��򿪱�����
int currentFile = -1;			//��ǰ�򿪵��ļ�
int currentFileIndex = -1;		//��ǰ�򿪵��ļ���Ŀ¼�򿪱�����
int currentFileUsedBlock = -1;

struct FCB fcb[256];
struct INode inode[256];
struct USEROPEN useropen[MAXOPEN_USER];
struct SYSTEMOPEN systemopen[MAXOPEN_SYSTEM];
struct blockBit blockMap[8000];

//==== ���� ====
int init_space()
{ //��ʼ���ļ��ռ�
	FILEBLOCK_START = malloc(SIZE);
	memset(FILEBLOCK_START, 0, SIZE);
	if (FILEBLOCK_START != NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//����λʾͼ
void update_blockMap(int blockNum, int newValue)
{
	blockMap[blockNum].val = newValue;
}

int get_blockMapValue(int blockNum)
{
	return blockMap[blockNum].val;
}

int block_free(int blockNum)
{
	if (get_blockMapValue(blockNum))
	{
		//��д��ָ��ƫ�� = FILEBLOCK_START + ƫ���� (���С*(����))
		memset(FILEBLOCK_START + blockNum * BLOCKSIZE, 0, BLOCKSIZE);
		//����λʾͼ
		update_blockMap(blockNum, 0);
		return 1;
	}
	else
	{
		return 0;
	}
}

int block_write(int blockNum, const void *data_start, int data_length)
{
	if (data_length > 512)
	{
		return 0;
	}
	if (!get_blockMapValue(blockNum))
	{
		//��д��ָ��ƫ�� = FILEBLOCK_START + ƫ���� (���С*(����))
		memcpy(FILEBLOCK_START + BLOCKSIZE * blockNum, data_start, data_length);
		update_blockMap(blockNum, 1);
		return 1;
	}
	else
	{
		return 0;
	}
}

void block_read(int blockNum)
{
	if (get_blockMapValue(blockNum))
	{
		//���ȡָ��ƫ�� = FILEBLOCK_START + ƫ���� (���С*(����))
		char *_start = (char *)(FILEBLOCK_START + blockNum * BLOCKSIZE);
		for (int i = 0; i < 256; i++)
		{
			if (*_start != 0){
				printf("%c", *_start);
			}
			_start++;
		}
	}
	else
	{
		printf("Block read error.");
	}
}

int getEmptyBlock()
{
	for (int i = 1; i <= 8000; i++)
	{
		if (!get_blockMapValue(i))
		{
			return i;
		}
	}
	return -1;
}

int getEmptyINode()
{
	for (int i = 0; i < 256; i++)
	{
		if (!inode[i].status)
		{
			return i;
		}
	}
	return -1;
}

int getEmptyUseropen()
{
	for (int i = 0; i < 50; i++)
	{
		if (!useropen[i].status)
		{
			return i;
		}
	}
	return -1;
}

int getEmptySystemopen()
{
	for (int i = 0; i < 50; i++)
	{
		if (!systemopen[i].status)
		{
			return i;
		}
	}
	return -1;
}

struct Directory *getCurrentDirectory()
{
	unsigned int b = inode[currentDirectory].directPointer[0]; //��ȡ����ǰĿ¼���ڵĿ��
	struct Directory *cd = (struct Directory *)(FILEBLOCK_START + BLOCKSIZE * b);
	return cd;
}

//��Ŀ¼
int create_root()
{
	//��Ŀ¼������FCB[0]��INode[0]
	inode[0].directPointer[0] = 0; //��1
	inode[0].indirectPointer = -1;
	inode[0].subIndirectPointer = -1;
	inode[0].attribute = 0;		  //�ļ���
	inode[0].status = 1;		  //���ռ��
	strcpy(fcb[0].filename, "/"); //�ļ���
	fcb[0].inodeIndex = 0;		  //�ļ���
	//д���Ŀ¼�ļ�����һ��
	struct Directory *directory = (struct Directory *)malloc(sizeof(Directory));
	directory->parentINodeIndex = 0;
	memset(directory->files, -1, sizeof(directory->files));
	return block_write(0, directory, sizeof(Directory));
}

//Ĭ��������򿪸�Ŀ¼
void open_root()
{
	systemopen[0].attribute = 0;
	strcpy(systemopen[0].filename, fcb[0].filename);
	systemopen[0].inodeIndex = 0;
	systemopen[0].shareCount = 1;
	systemopen[0].status = 1;
	useropen[0].status = 1;
	useropen[0].openMode = 2;
	useropen[0].systemOpenEntry = 0;
	currentDirectory = 0;
	currentDirectoryIndex = 0;
}

//��ӡ��ǰĿ¼
void printCurrentDirectory()
{
	char buffer[512] = {0};
	char buffer_final[512] = {0};
	char s[512] = {0};
	int dIndex = useropen[currentDirectoryIndex].systemOpenEntry;
	int dInode = systemopen[dIndex].inodeIndex;
	while (dIndex != 0)
	{
		snprintf(buffer, sizeof(buffer), "%s%s", systemopen[dIndex].filename, "/");
		int len = strlen(buffer);
		int len_final = strlen(buffer_final);
		memcpy(s, buffer, len * sizeof(char));
		memcpy(s + len * sizeof(char), buffer_final, len_final * sizeof(char));
		memcpy(buffer_final, s, sizeof(s));
		memset(s, 0, sizeof(s));
		dIndex = systemopen[dIndex].parentIndex;
		dInode = systemopen[dIndex].inodeIndex;
	}
	printf("/%s", buffer_final);
	printf("%s", " # ");
	if (currentFile >= 0)
	{
		printf("(%s) ", fcb[currentFile].filename);
	}
}

//��ǰĿ¼����ļ�
int addfile_directory(int fileId)
{
	struct Directory *cd = getCurrentDirectory();
	int emptyIndex = -1;
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] == -1)
		{
			emptyIndex = i;
			break;
		}
	}
	if (emptyIndex < 0)
	{
		return 0;
	}
	cd->files[emptyIndex] = fileId;
	return 1;
}

//�ӵ�ǰĿ¼�Ƴ��ļ�
int removefile_directory(int fileId)
{
	struct Directory *cd = getCurrentDirectory();
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] == fileId)
		{
			cd->files[i] = -1;
			return 1;
		}
	}
	return 0;
}

//�г���ǰĿ¼������
void list_directory()
{
	struct Directory *directory = getCurrentDirectory();
	int count = 0;
	for (int i = 0; i < 127; i++)
	{
		if (directory->files[i] != -1)
		{
			if (inode[directory->files[i]].attribute == 0)
			{ //���ļ���
				printf("[D]%s\t", fcb[directory->files[i]].filename);
			}
			else
			{
				printf("%s\t", fcb[directory->files[i]].filename); //���ļ�
			}
			count++;
			if (count % 6 == 0)
			{
				printf("\n");
			}
		}
	}
	if (count == 0)
	{
		printf("There are no files under this folder.\n");
	}
	else
	{
		printf("\n");
	}
}

//�����µ��ļ���
int make_directory(char *name)
{
	//�������
	struct Directory *cd = getCurrentDirectory();
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] != -1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0 && inode[cd->files[i]].attribute == 0)
			{
				//��������
				return 0;
			}
		}
	}
	int inodeId = getEmptyINode(); //��ȡ�յ�INode�ڵ�
	if (inodeId >= 0)
	{
		int blockNum = getEmptyBlock();
		if (blockNum >= 0)
		{
			//д��Ŀ¼�ļ�
			strcpy(fcb[inodeId].filename, name);
			fcb[inodeId].inodeIndex = inodeId;
			inode[inodeId].attribute = 0;
			inode[inodeId].status = 1;
			inode[inodeId].directPointer[0] = blockNum;
			inode[inodeId].indirectPointer = -1;
			inode[inodeId].subIndirectPointer = -1;
			//���µ�ǰĿ¼
			if (!addfile_directory(inodeId))
			{
				//����ʧ��ʱ�ͷŷ����fcb
				memset(&inode[inodeId], 0, sizeof(inode[inodeId]));
				memset(&fcb[inodeId], 0, sizeof(fcb[inodeId]));
				return 0;
			}
			//д��Ŀ¼���ļ��ռ�
			struct Directory *directory = (struct Directory *)malloc(sizeof(Directory));
			directory->parentINodeIndex = currentDirectory;
			memset(directory->files, -1, sizeof(directory->files));
			return block_write(blockNum, directory, sizeof(Directory));
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//������Ŀ¼
int enter_directory(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 0)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				break;
			}
		}
	}
	if (file < 0)
	{
		return 0;
	}

	for (int i = 0; i < 50; i++)
	{ //����ļ��ǲ����Ѿ�����
		if (systemopen[i].inodeIndex == file)
		{
			systemopen[i].shareCount++;
			int emptyUseropen = getEmptyUseropen();
			if (emptyUseropen < 0)
			{
				return 0;
			}
			useropen[emptyUseropen].status = 1;
			useropen[emptyUseropen].systemOpenEntry = i;
			useropen[emptyUseropen].openMode = 2;
			return 1;
		}
	}

	//û�д�
	int emptySystemopen = getEmptySystemopen();
	int emptyUseropen = getEmptyUseropen();
	if (emptySystemopen < 0 || emptyUseropen < 0)
	{
		return 0;
	}

	//���뵽�ļ��򿪱�
	systemopen[emptySystemopen].attribute = 0;
	strcpy(systemopen[emptySystemopen].filename, fcb[file].filename);
	systemopen[emptySystemopen].inodeIndex = fcb[file].inodeIndex;
	systemopen[emptySystemopen].status = 1;
	systemopen[emptySystemopen].shareCount = 1;
	systemopen[emptySystemopen].parentIndex = useropen[currentDirectoryIndex].systemOpenEntry;
	systemopen[emptySystemopen].parentINode = currentDirectory;
	useropen[emptyUseropen].status = 1;
	useropen[emptyUseropen].systemOpenEntry = emptySystemopen;
	useropen[emptyUseropen].openMode = 2;
	useropen[emptyUseropen].parentIndex = currentDirectoryIndex;
	//����ȫ��
	currentDirectory = file;
	currentDirectoryIndex = emptyUseropen;
}

//�ص��ϼ�Ŀ¼
int back_parentDirectory()
{
	//��ȡ��Ŀ¼�����Ϣ
	unsigned short parentInode = systemopen[useropen[currentDirectoryIndex].systemOpenEntry].parentINode;
	unsigned short parentIndex = useropen[currentDirectoryIndex].parentIndex;
	//�رյ�ǰĿ¼�ļ�
	useropen[currentDirectoryIndex].status = 0;
	systemopen[useropen[currentDirectoryIndex].systemOpenEntry].shareCount--;
	if (systemopen[useropen[currentDirectoryIndex].systemOpenEntry].shareCount <= 0)
	{
		memset(&systemopen[useropen[currentDirectoryIndex].systemOpenEntry], 0, sizeof(SYSTEMOPEN));
	}
	//����
	memset(&useropen[currentDirectoryIndex], 0, sizeof(USEROPEN));
	//����ȫ��
	currentDirectory = parentInode;
	currentDirectoryIndex = parentIndex;
	return 1;
}

//ɾ��Ŀ¼
int delete_directory(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	int fileIndex = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 0)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				fileIndex = i;
				break;
			}
		}
	}
	if (file < 0)
	{
		printf("Directory not found.\n");
		return 1;
	}
	//��ȡĿ��Ŀ¼�ļ�
	struct Directory *target = (struct Directory *)(FILEBLOCK_START + BLOCKSIZE * inode[file].directPointer[0]);
	for (int i = 0; i < 127; i++)
	{
		if (target->files[i] != -1)
		{
			//Ŀ¼�´����ļ�
			int id = target->files[i];
			printf("Can not delete the directory because some files are still under it.\n");
			return 1;
		}
	}
	//ɾ��Ŀ¼�ļ���
	if (!block_free(inode[file].directPointer[0]))
	{
		return 0;
	}
	//�ͷ�fcb
	memset(&fcb[file], 0, sizeof(fcb[file]));
	memset(&inode[file], 0, sizeof(inode[file]));
	//���µ�ǰĿ¼
	cd->files[fileIndex] = -1;
	return 1;
}

//�����ļ�
int create_file(char *name)
{
	//�������
	struct Directory *cd = getCurrentDirectory();
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] != -1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0 && inode[cd->files[i]].attribute == 1)
			{
				//��������
				return 0;
			}
		}
	}
	//����inode
	int inodeId = getEmptyINode(); //��ȡ�յ�INode�ڵ�
	if (inodeId >= 0)
	{
		//д���ļ�ʱ������̿ռ䣬����������
		inode[inodeId].attribute = 1;
		inode[inodeId].status = 1;
		strcpy(fcb[inodeId].filename, name);
		fcb[inodeId].inodeIndex = inodeId;
		//ָ���ʼ��
		for (int i = 0; i < 8; i++)
		{
			inode[inodeId].directPointer[i] = -1;
		}
		inode[inodeId].indirectPointer = -1;
		inode[inodeId].subIndirectPointer = -1;
		//���µ�ǰ�ļ���
		if (!addfile_directory(inodeId))
		{
			//����ʧ��ʱ�ͷŷ����fcb
			memset(&inode[inodeId], 0, sizeof(inode[inodeId]));
			memset(&fcb[inodeId], 0, sizeof(fcb[inodeId]));
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//ɾ���ļ�
int remove_file(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	int fileIndex = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				fileIndex = i;
				break;
			}
		}
	}
	if (file < 0)
	{
		printf("File not found.\n");
		return 1;
	}
	//ѭ���ͷ�Ŀ���ļ����ļ���
	for (int i = 0; i < 8; i++)
	{
		if (inode[file].directPointer[i] >= 0)
		{
			block_free(inode[file].directPointer[i]);
		}
	}
	//һ��ָ����
	if (inode[file].indirectPointer >= 0)
	{
		struct INodeBlock *block = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[file].indirectPointer);
		for (int i = 0; i < 128; i++)
		{
			//ָ�뱻ռ�ã��ͷŶ�Ӧ��
			if (block->pointers[i] >= 0)
			{
				block_free(block->pointers[i]);
			}
		}
		//�ͷŸ�ָ���
		block_free(inode[file].indirectPointer);
	}
	//����ָ����
	if (inode[file].subIndirectPointer >= 0)
	{
		struct INodeBlock *block = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[file].subIndirectPointer);
		for (int i = 0; i < 128; i++)
		{
			if (block->pointers[i] >= 0)
			{
				struct INodeBlock *subBlock = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * block->pointers[i]);
				for (int j = 0; j < 128; j++)
				{
					if (subBlock->pointers[j] >= 0)
					{
						block_free(subBlock->pointers[j]);
					}
				}
				//�ͷŵ�ǰ��ָ���
				block_free(block->pointers[i]);
			}
		}
		//�ͷ�ָ���
		block_free(inode[file].subIndirectPointer);
	}
	//����fcb
	memset(&fcb[file], 0, sizeof(fcb[file]));
	memset(&inode[file], 0, sizeof(inode[file]));
	//���µ�ǰĿ¼
	cd->files[fileIndex] = -1;
	return 1;
}

//���ļ�
int open_file(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				break;
			}
		}
	}
	if (file < 0)
	{
		printf("File not found.\n");
		return 1;
	}

	for (int i = 0; i < 50; i++)
	{ //����ļ��ǲ����Ѿ�����
		if (systemopen[i].inodeIndex == file)
		{
			systemopen[i].shareCount++;
			int emptyUseropen = getEmptyUseropen();
			if (emptyUseropen < 0)
			{
				return 0;
			}
			useropen[emptyUseropen].status = 1;
			useropen[emptyUseropen].systemOpenEntry = i;
			useropen[emptyUseropen].openMode = 2;
			return 1;
		}
	}

	//û�д�
	int emptySystemopen = getEmptySystemopen();
	int emptyUseropen = getEmptyUseropen();
	if (emptySystemopen < 0 || emptyUseropen < 0)
	{
		return 0;
	}

	//���뵽�ļ��򿪱�
	systemopen[emptySystemopen].attribute = 0;
	strcpy(systemopen[emptySystemopen].filename, fcb[file].filename);
	systemopen[emptySystemopen].inodeIndex = fcb[file].inodeIndex;
	systemopen[emptySystemopen].status = 1;
	systemopen[emptySystemopen].shareCount = 1;
	systemopen[emptySystemopen].parentIndex = useropen[currentDirectoryIndex].systemOpenEntry;
	systemopen[emptySystemopen].parentINode = currentDirectory;
	useropen[emptyUseropen].status = 1;
	useropen[emptyUseropen].systemOpenEntry = emptySystemopen;
	useropen[emptyUseropen].openMode = 2;
	useropen[emptyUseropen].parentIndex = currentDirectoryIndex;
	//����ȫ��
	currentFile = file;
	currentFileIndex = emptyUseropen;
	currentFileUsedBlock = (int)ceil((double)inode[file].length / BLOCKSIZE);
	return 1;
}

//�ر��ļ�
int close_file()
{
	//�رյ�ǰ�ļ�
	useropen[currentFileIndex].status = 0;
	systemopen[useropen[currentFileIndex].systemOpenEntry].shareCount--;
	if (systemopen[useropen[currentFileIndex].systemOpenEntry].shareCount <= 0)
	{
		memset(&systemopen[useropen[currentFileIndex].systemOpenEntry], 0, sizeof(SYSTEMOPEN));
	}
	//����
	memset(&useropen[currentFileIndex], 0, sizeof(USEROPEN));
	//����ȫ��
	currentFile = -1;
	currentFileIndex = -1;
	currentFileUsedBlock = -1;
}

//д�ļ�
int write_file(char *content)
{
	if (currentFile >= 0)
	{
		//�ļ����ݲ�����
		int len = strlen(content) * sizeof(char); //��ȡд�����ݵĳ���
		int offset = 0;
		void *p = (void *)content;
		while (len - offset > 0)
		{
			int bs = BLOCKSIZE;
			if (len - offset < BLOCKSIZE)
			{
				//���Ȳ���һ��BLOCK����ֻȡ�䳤��
				bs = len;
			}
			//ȡ��Ҫд������ݷŵ�������
			void *buffer = malloc(bs);
			memcpy(buffer, p + offset, bs);
			//������д�뵽�ļ��ռ�
			if (currentFileUsedBlock >= 136)
			{
				//�Ѿ�ʹ���˳���136�����ݣ�Ӧ��д�뵽�������ָ���µ�block
				if (currentFileUsedBlock == 136)
				{
					//����������ָ��
					int b_subinode = getEmptyBlock();
					if (b_subinode >= 0)
					{
						struct INodeBlock *ib = (struct INodeBlock *)malloc(sizeof(INode));
						block_write(b_subinode, ib, sizeof(INode));
						inode[currentFile].subIndirectPointer = b_subinode;
					}
					else
					{
						printf("Failed, no more empty block.");
						return 1;
					}
				}
				int subUsedBlock = currentFileUsedBlock - 136;
				int i_sub = (int)ceil((double)subUsedBlock / 128);																	   //����������λ��
				int i_subib = subUsedBlock % 128;																					   //����������������ڵ�λ��
				struct INodeBlock *subib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].subIndirectPointer); //�������ָ���������
				//ÿ128����Ҫ����һ���µ��ӿ�
				if (subUsedBlock % 8 == 0)
				{
					//�����Ӽ��ָ���������
					int b_inode = getEmptyBlock();
					if (b_inode >= 0)
					{
						struct INodeBlock *ib = (struct INodeBlock *)malloc(sizeof(INode));
						block_write(b_inode, ib, sizeof(INode));
						subib->pointers[i_sub] = b_inode;
					}
					else
					{
						printf("Failed, no more empty block.");
						return 1;
					}
				}
				//д��buffer������
				int b = getEmptyBlock();
				if (b >= 0)
				{
					block_write(b, buffer, bs); //��bufferд�뵽�ļ�
					struct INodeBlock *c_ib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * subib->pointers[i_sub]);
					c_ib->pointers[i_subib] = b;
					inode[currentFile].length += bs;
					currentFileUsedBlock++;
				}
				else
				{
					printf("Failed, no more empty block.");
					return 1;
				}
			}
			else if (currentFileUsedBlock >= 8)
			{
				//�Ѿ�ʹ���˳���8�����ݣ�Ӧ��д�뵽һ�����ָ���µ�block
				if (currentFileUsedBlock == 8)
				{
					//����һ�����ָ��
					int b_inode = getEmptyBlock();
					if (b_inode >= 0)
					{
						struct INodeBlock *ib = (struct INodeBlock *)malloc(sizeof(INode));
						block_write(b_inode, ib, sizeof(INode));
						inode[currentFile].indirectPointer = b_inode;
					}
					else
					{
						printf("Failed, no more empty block.");
						return 1;
					}
				}
				int b = getEmptyBlock();
				if (b >= 0)
				{
					block_write(b, buffer, bs); //��bufferд�뵽�ļ�
					struct INodeBlock *indexBlock = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].indirectPointer);
					indexBlock->pointers[currentFileUsedBlock - 8] = b;
					inode[currentFile].length += bs;
					currentFileUsedBlock++;
				}
				else
				{
					printf("Failed, no more empty block.");
					return 1;
				}
			}
			else
			{
				//δ����8�飬д�뵽ֱ��ָ��
				int b = getEmptyBlock();
				if (b >= 0)
				{
					block_write(b, buffer, bs); //��bufferд�뵽�ļ�
					inode[currentFile].directPointer[currentFileUsedBlock] = b;
					inode[currentFile].length += bs;
					currentFileUsedBlock++;
				}
				else
				{
					printf("Failed, no more empty block.");
					return 1;
				}
			}
			//�ͷŻ�����
			free(buffer);
			//���±���
			offset += bs;
		}
	}
	else
	{
		return 0;
	}
	return 1;
}

//���ļ�
int read_file()
{
	if (currentFile >= 0)
	{
		if (currentFileUsedBlock == 0)
		{
			printf("File is empty.\n");
			return 1;
		}
		for (int i = 0; i < 8; i++)
		{
			if (inode[currentFile].directPointer[i] >= 0)
			{
				block_read(inode[currentFile].directPointer[i]);
			}
			else
			{
				break;
			}
		}
		if (inode[currentFile].indirectPointer >= 0)
		{
			struct INodeBlock *ib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].indirectPointer);
			for (int i = 0; i < 128; i++)
			{
				if (ib->pointers[i] >= 0)
				{
					block_read(ib->pointers[i]);
				}
				else
				{
					break;
				}
			}
		}
		if (inode[currentFile].subIndirectPointer >= 0)
		{
			struct INodeBlock *subib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].subIndirectPointer);
			for (int i = 0; i < 128; i++)
			{
				if (subib->pointers[i] >= 0)
				{
					struct INodeBlock *ib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * subib->pointers[i]);
					for (int j = 0; j < 128; j++)
					{
						if (ib->pointers[j] >= 0)
						{
							block_read(ib->pointers[j]);
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		return 0;
	}
	printf("\n");
	return 1;
}

void cat_file(char *name){
	if (open_file(name)){
		read_file();
		close_file();
	}
}