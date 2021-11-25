/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/** @file main.cpp
 *  @brief This contains a method to transform csv files into mrmr binary files.
 *
 *  @author Iago Lastra (IagoLast). Modified by Andrew Guy (2021).
 */
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <algorithm>
#include "StringTokenizer.h"
#include <map>

using namespace std;
typedef unsigned char byte;

/**
 * 	@brief Function to check if a key apears in a map.
 *	@param key the key that you are looking for
 *	@param mymap the map where you are going to check.
 *	@returns A boolean value depending if the key apears or not.
 */
bool contains(string key, map<string, byte> mymap) {
	map<string, byte>::iterator it = mymap.find(key);
	if (it == mymap.end()) {
		return false;
	}
	return true;
}


class InputParser{
    public:
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(std::string(argv[i]));
        }
        /// @author iain
        const std::string& getCmdOption(const std::string &option) const{
            std::vector<std::string>::const_iterator itr;
            itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const std::string empty_string("");
            return empty_string;
        }
        /// @author iain
        bool cmdOptionExists(const std::string &option) const{
            return std::find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }
    private:
        std::vector <std::string> tokens;
};

void print_help() {
		printf("MRMR Reader for converting CSV file to binary format for use with fast-mRMR.\n\n");
        printf("Usage: -f INPUT -o OUTPUT [--gpu]\n\n");
		printf("Note: If --gpu flag is set, will discard last (n modulo 16) datapoints, where n is the total number of datapoints.\n");
		exit(-1);
}

/**
 *@brief Translates a csv file into a binary file.
 *@brief Each different value will be mapped into an integer
 *@brief value from 0 to 255.
 */
int main(int argc, char* argv[]) {
	uint datasize = 0;
	uint featuresSize = 0;
	uint featurePos = 0;
	uint i = 0;
	uint limitMod16 = 0;
	string line;
	string token;
	byte data;
	vector<byte> lastCategory;
	vector<map<string, byte> > translationsVector;

//Parse console arguments.
	InputParser input(argc, argv);
	if(input.cmdOptionExists("-h")){
		print_help();
    }

	const std::string &inputFilename = input.getCmdOption("-f");
	const std::string &outputFilename = input.getCmdOption("-o");
	const bool for_gpu = input.cmdOptionExists("--gpu");

    if (inputFilename.empty() || outputFilename.empty()){
		printf("Please provide input and output filenames. See below for usage instructions: \n\n");
        print_help();
    }

	ifstream inputFile(inputFilename);
	ofstream outputFile(outputFilename, ios::out | ios::binary);

//Count lines and features.
	if (inputFile.is_open()) {
//Ignore first line (headers from csv)
		getline(inputFile, line);
		StringTokenizer strtk(line, ",");
		while (strtk.hasMoreTokens()) {
			token = strtk.nextToken();
			featuresSize++;
		}
		while (getline(inputFile, line)) {
			++datasize;
		}

		if (for_gpu) {
			//Only %16 datasize can be computed on GPU.
			limitMod16 = (datasize % 16);
			datasize = datasize - limitMod16;
		}

//write datasize and featuresize:
		outputFile.write(reinterpret_cast<char*>(&datasize), sizeof(datasize));
		outputFile.write(reinterpret_cast<char*>(&featuresSize), sizeof(featuresSize));

//Set pointer into beginning of the file.
		inputFile.clear();
		inputFile.seekg(0, inputFile.beg);

//Initialize translation map.
		for (i = 0; i <= featuresSize; ++i) {
			map<string, byte> map;
			translationsVector.push_back(map);
			lastCategory.push_back(0);
		}

//Ignore first line (headers from csv) again.
		getline(inputFile, line);
		uint lineCount = 0;
//Read and translate file to binary.
		while (getline(inputFile, line)) {
			if (lineCount == (datasize)) {
				if (limitMod16 != 0) {
					printf("Last %d samples ignored.\n", limitMod16);
				}
				break;
			}
			featurePos = 0;
			StringTokenizer strtk(line, ",");
			while (strtk.hasMoreTokens()) {
				token = strtk.nextToken();
				featurePos++;
				if (!contains(token, translationsVector[featurePos])) {
					translationsVector[featurePos][token] =
							lastCategory[featurePos];
					lastCategory[featurePos]++;
				}
				data = translationsVector[featurePos][token];
				outputFile.write(reinterpret_cast<char*>(&data), sizeof(byte));
			}
			lineCount++;
		}
		outputFile.flush();
		outputFile.close();
		inputFile.close();
	} else {
		cout << "Error loading file.\n";
		exit(-1);
	}
	return 0;

}
