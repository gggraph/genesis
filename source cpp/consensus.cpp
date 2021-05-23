#include "consensus.h"
#include "sha256.h"

void ProccessBlocksFile(const char * filePath)
{
	// create virtual utxo set 
	FILE* f = fopen("utxos\\tmp", "ab");
	if (f == NULL)
	{
		std::cout << "[Block Refused] Cannot create tmp file";  return;
	}
	fclose(f);


	uint32_t lastOfficialindex = GetLatestBlockIndex(true);
	// firstindex . lastindex.  blocksptr (4o) . data
	unsigned char buff[8];
	ReadFile(filePath, 0, 8, buff);
	uint32_t firstfblockindex = BytesToUint(buff);
	uint32_t lastfblockindex = BytesToUint(buff + 4);
	std::cout << "first block index " << firstfblockindex << std::endl;
	std::cout << "last block index " << lastfblockindex << std::endl;
	if (!VerifyBlocksFile(filePath, lastOfficialindex, firstfblockindex, lastfblockindex)) 
	{
		// clear temp file
		remove("utxos\\tmp");
		remove(filePath);
	}
	else
	{
		std::cout << "comparing " << lastfblockindex << " and " << lastOfficialindex + RUN_DISTANCE << std::endl;
		if ( lastfblockindex >= lastOfficialindex + RUN_DISTANCE )
		{
			//update the chain...
			// IT IS A WIN !
			std::cout << "_____________[BLOCKCHAIN WILL BE UPDATED]____________";
			
			UpdateBlockchain(filePath);
		}
		else
		{
			//put in in fork
			size_t oldpsize = strlen(filePath); // path name is tmp\\ need to move it to fork ...
			std::ostringstream s;
			s << "fork";
			s << std::string(filePath, 3, oldpsize);
			std::string ss = s.str();
			std::cout <<"renaming : " << ss << std::endl;
			int result = rename(filePath, ss.c_str());
			if (result == 0)
				puts("Blocks file moved");
			else
				perror("Error renaming file");

			remove("utxos\\tmp");
			return;

		}
	}
}

void UpdateBlockchain(const char * filePath)
{
	// [0] Update utxo set with temp utxo // seems not working 
	UpdateUtxoSet();
	// [0a] [MINER] Delete PTX based on pukey and TOU based on every TX
	RefreshPTXFileFromVirtualUtxoSet(); //< gives error reading seems ok ( 0 bytes ) 
	// [1] Delete temp utxo
	remove("utxos\\tmp");
	// [2] Procceed SmartContract Request (x) SmartContract Submission
	//...
	// [3] Append Block Data to official chain 
	UpdateOfficialChain(filePath);
	// [4] Delete Temp Block File
	remove(filePath);
	// [5] Empty Fork Directory
	DeleteDirectory("fork");
	_wmkdir(L"fork");
	// [6] Refresh PTRS file and Blocks Cache
	LoadBlockPointers();
	// CONGRATS
	std::cout << "congrats! " << std::endl;

}

bool VerifyBlocksFile(const char * filePath, uint32_t lastOfficialindex, uint32_t firstfblockindex, uint32_t lastfblockindex) 
// need to work with 4gb++ size file
{
	std::cout << "Start validating " << filePath;
;
	unsigned char buff[8];
	if (lastfblockindex <= lastOfficialindex ) // except if files is split (4gb max ... need to handle this next time ) 
	{
		std::cout << "[Block Refused] Lower index than official" << std::endl;
		return false;
	}
	if (firstfblockindex-1 > lastOfficialindex)
	{
		std::cout << "[Block Refused] Can't proccess block. Index too high." << std::endl;
		return false;
	}

	if (firstfblockindex == 0 )
	{
		std::cout << "[Block Refused] Genesis not allowed" << std::endl;
		return false;
	}
	unsigned char * cBlock;
	unsigned char * lBlock;
	// verify first block 
	for (int i = firstfblockindex; i <= lastfblockindex; i++ )
	{
		std::cout << "Validating block # " << i << std::endl;;
		// load block in memory (2mb max ) 
		if ( i == firstfblockindex)
		{
			std::cout << "reading official block # " << (i-1) << std::endl;;
			lBlock = GetOfficialBlock(i - 1);
		}
		else
		{
			std::cout << "reading unofficial block # " << (i - 1) << std::endl;;
			lBlock = GetUnofficialBlock(filePath, i - 1);
		}
			

		cBlock = GetUnofficialBlock(filePath, i);
		
		if ( cBlock == NULL || lBlock == NULL){
			std::cout << "[Block Refused] Block not found in file or file corrupted." << std::endl; 
			return false;
		}
		// verify index corectness
		if ( GetBlockIndex(cBlock) != i )
		{
			std::cout << "[Block Refused] Invalid index." << std::endl;
			free(lBlock);
			free(cBlock);
			return false;
		}
		
		// get timestamp requirement 
		uint32_t MIN_TIMESTAMP = GetRequiredTimeStamp(i, firstfblockindex, lBlock, cBlock, filePath);
		if ( MIN_TIMESTAMP == 0 )
		{
			std::cout << "[Block Refused] Can't verify timestamp" << std::endl;
			free(lBlock);
			free(cBlock);
			return false;
		}

		// get hash target requirement
		unsigned char reqtarget[32];
		GetRequiredTarget(reqtarget, firstfblockindex, filePath);
		
		// get b size [ shitty but workin ? ]

		// there is a problem with firstfblockindex ... 
		uint32_t boff1, boff2;
		ReadFile(filePath, 8 + (i-firstfblockindex)*4, 4, buff);
		std::cout << "will get bloc#" << (i) << " offset at " << (8 + ((i) - firstfblockindex) * 4) << std::endl;
		boff1 = BytesToUint(buff);
		if ( i != lastfblockindex )
		{
			std::cout << "will get bloc#" << (i + 1) << " offset at " << (8 + ((i+1) - firstfblockindex) * 4) << std::endl;
			ReadFile(filePath, 8 + (((i + 1) - firstfblockindex) * 4), 4, buff);
			boff2 = BytesToUint(buff);
		}
		else
		{
			FILE* f = fopen(filePath, "rb");
			if (f == NULL) { 
				std::cout << "error reading..." << std::endl;
				free(lBlock);
				free(cBlock);
				return false; } // throw error if cannot read
			fseek(f, 0, SEEK_END);
			long fsize = ftell(f);
			fclose(f);
			boff2 = fsize;
		
		}
		std::cout << "current block offset" << boff1 << std::endl;
		std::cout << "next block offset" << boff2 << std::endl; // problem here 
		uint32_t bsize = boff2 - boff1;
		std::cout << "block size is " << bsize << std::endl;
		// verify 
		if (!IsBlockValid(cBlock,lBlock,firstfblockindex,MIN_TIMESTAMP, bsize, reqtarget))
		{
			free(lBlock);
			free(cBlock);
			return false;
		}
		
		free(lBlock);
		free(cBlock);
	}
	std::cout << "Blocks file is valid." << std::endl;
	return true;
	
}
void GetRequiredTarget(unsigned char * buff, uint32_t firstbfIndex, const char * filePath)
{
	memset(buff, 0, 32);
}

uint32_t GetRequiredTimeStamp(int index , uint32_t firstbfIndex, unsigned char * lBlock, unsigned char * cBlock, const char * filePath )
{
	uint32_t result = 0;

	if (index - (TIMESTAMP_TARGET / 2) < firstbfIndex || (TIMESTAMP_TARGET / 2) > index)
	{
		unsigned char * tBlock = NULL;
		if ((TIMESTAMP_TARGET / 2) > index)
			tBlock = GetOfficialBlock(0);
		else
			tBlock = GetOfficialBlock(index - (TIMESTAMP_TARGET / 2));

		if (tBlock == NULL)
		{
			std::cout << "[Block Refused] Cannot proccess timestamp requirement [A]" << std::endl;
			return 0;
		}
		else {
			result = GetBlockTimeStamp(tBlock);
			free(tBlock);
		}
	}
	else
	{
		unsigned char * tBlock = GetUnofficialBlock(filePath, index - (TIMESTAMP_TARGET / 2));
		if (tBlock == NULL)
		{
			std::cout << "searching block #" << (TIMESTAMP_TARGET / 2) << "in inofficial" << std::endl;
			std::cout << "[Block Refused] Cannot proccess timestamp requirement [B]" << std::endl;
			return 0;

		}
		result =  GetBlockTimeStamp(tBlock);
		free(tBlock);

	}
	return result;
}


//IsBlockValid(Block b, Block prevb, uint MIN_TIME_STAMP, byte[] HASHTARGET, byte[] reqtarget, List<UTXO> vUTXO)
bool IsBlockValid(unsigned char * b, unsigned char * prevb, uint32_t firstblockindex, uint32_t MIN_TIME_STAMP, uint32_t bsize, unsigned char * reqtarget)
{
	// [0] verify hash root 
	unsigned char * ublock = (unsigned char *)malloc(bsize - 36);
	unsigned char buff[32];
	// memcpy everything except hash & nonce 
	/*
	Reminder : index (4o) . hash (32o) . phash (32o) . timestamp (4o) . hashtarget (32o) .  nonce (4 o) .  miner token [can be either 4+1 o or 532+1 o] .
	. txn ( 2o ) . txs (variable)
	*/
	memcpy(ublock, b, 4);
	memcpy(ublock+4, b+36, 68);
	memcpy(ublock + 72, b + 108, bsize - 108);
	Sha256.init();
	Sha256.write((char *)ublock, bsize - 36);
	memcpy(buff, Sha256.result(), 32);
	if (memcmp(buff, GetBlockHash(b), 32) != 0)
	{
		std::cout << "[Block Refused] Wrong hash root" << std::endl; 
		printHash(buff);
		printHash(GetBlockHash(b));
		free(ublock);
		return false;
	}
		
	free(ublock);

	// [1] verify previous hash
	if ( memcmp(GetBlockPreviousHash(b), GetBlockHash(prevb), 32) != 0)
	{
		
		std::cout << "[Block Refused] Wrong previous hash" << std::endl;
		printHash(GetBlockHash(prevb));
		printHash(GetBlockPreviousHash(b));

		return false;
	}
		
	// [2] verify timestamp 
	uint32_t bts = GetBlockTimeStamp(b);
	uint32_t cts = GetTimeStamp();
	if ( bts < MIN_TIME_STAMP || bts > cts + MAX_TIME_UP)
	{
		std::cout << "[Block Refused] Wrong time stamp" << std::endl; 
		return false;
	}
		
	// [3] verify nonce x hashtarget 
	//.... (not verified 4 da moment )
	// [4] verify every tx 
	uint32_t mReward = GetMiningReward(GetBlockIndex(b));
	for (int i = 0 ; i < GetTransactionNumber(b); i++ )
	{
		unsigned char * txp = GetBlockTransaction(b, i); // don't need to free it. it's in b mem.
		if ( !IsTransactionValid(txp, firstblockindex - 1) )
		{
			std::cout << "[Block Refused] Transaction not valid." << std::endl; 
			return false;
		
		}
		mReward += GetTXFee(txp);
	}
	// [5] Update miner virtual utxo with reward + mining reward (if no dust )
	// PROBLEM HERE 
	unsigned char  mutxo[544];
	memset(mutxo, 0, 540);
	if ( GetMinerTokenFlag(b) == 1 ) // +108
	{
		unsigned char nutxo[540];
		memcpy(nutxo, b + 109, 532);
		memset(nutxo + 532, 0, 8);
		GetVirtualUtxo(0, firstblockindex - 1, mutxo, nutxo); 
		mReward /= 2; // PENALITY FOR NEW MINER ( REAL HARD ? )
	
	}
	else
	{
		uint32_t mutxop = BytesToUint(b + 109);
		GetVirtualUtxo(mutxop, firstblockindex - 1, mutxo);
	}
	
	if (isUtxoNull(mutxo)) // never happen ... cause utxo never null
	{
		std::cout << "[Block Refused] Cannot proccess miner UTXO." << std::endl; 
		return false;
	}
	UintToBytes(GetUtxoSold(mutxo) + mReward, mutxo + 536);
	std::cout << GetUtxoSold(mutxo) << std::endl; 
	OverWriteVirtualUtxo(mutxo);

	return true;
}


uint32_t GetMiningReward(uint32_t index )
{
	uint32_t Reward = NATIVE_REWARD;
	while (index >= HALVING_CLOCK)
	{
		index -= HALVING_CLOCK;
		Reward /= 2;
	}
	return Reward;
}

void ComputeHashTarget(uint32_t index, unsigned char * buff) // need a buffer[32]
{
	//get block at index-TARGETCLOCK and do the common things with 256b number 
	memset(buff, 0, 32);
}

void GetRelativeHashTarget(uint32_t index, unsigned char * buff) // needed when 
{
	uint32_t ni = nearestmultiple(index, TARGET_CLOCK, true);
	unsigned char * b = GetOfficialBlock(ni);
	memcpy(buff, GetBlockHashTarget(b), 32);
	free(b);
}