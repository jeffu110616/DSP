
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include "Ngram.h"

typedef struct _Node{
    string word;
    string sentence;
    struct _Node *prev;
    double prob;
} DPNode;

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

double getBigramProb(const char *w1, const char *w2, Vocab &voc, Ngram &lm){

    VocabIndex wid1 = voc.getIndex(w1);
    VocabIndex wid2 = voc.getIndex(w2);

    if(wid1 == Vocab_None){  //OOV
        wid1 = voc.getIndex(Vocab_Unknown);
    }
    
    if(wid2 == Vocab_None){  //OOV
        wid2 = voc.getIndex(Vocab_Unknown);
    }
    
    VocabIndex context[] = { wid1, Vocab_None };
    
    return lm.wordProb( wid2, context );
}

int main(int argc, char *argv[]){

    // read arguments
    InputParser input(argc, argv);
    const string &f_text = input.getCmdOption("-text");
    const string &f_lm = input.getCmdOption("-lm");
    const string &f_map = input.getCmdOption("-map");

    int ngram_order = 2;
    Vocab voc;
    Ngram lm( voc, ngram_order );

    {
        // const char lm_filename[] = "./bigram.lm";
        File lmFile( f_lm.c_str(), "r" );
        lm.read( lmFile );
        lmFile.close();
    }

    // cout << getBigramProb( "<s>", "èÙ", voc, lm ) << endl;

    // building ZhuYin mapping 
    string line;
    // ifstream mapfile("ZhuYin-Big5.map");
    ifstream mapfile( f_map.c_str() );
    map< string, vector<string> > z_map;

    size_t max_mapping_length = 0;
    if(mapfile.is_open()){
        // int counter = 0;
        
        while ( getline(mapfile, line) ){
            // if(++counter > 4) break;

            string key, rest;
            key = line.substr(0, line.find("\t"));
            rest = line.substr(line.find("\t")+1, string::npos);

            // parse toks
            size_t pos = 0;
            string delimiter = " ";
            string tok;
            vector<string> vec_tmp;
            while ((pos = rest.find(delimiter)) != string::npos) {
                tok = rest.substr(0, pos);
                // store to map
                // cout << "[" << tok << "]" << endl;
                // vec_tmp.insert( vec_tmp.end(), tok );
                vec_tmp.push_back( tok );
                rest.erase(0, pos + delimiter.length());
            }
            tok = rest.substr(0, rest.find("\n"));
            // vec_tmp.insert( vec_tmp.end(), tok );
            vec_tmp.push_back( tok );
            z_map[key] = vec_tmp;
            // cout << "key: " << key << "\n\trest: " << rest << endl;
        }
        mapfile.close();
    }else cout << "Unable to open file"; 
    // add </s> mapping
    {
        string tmp_str = "</s>";
        vector<string> tmp_vec;
        tmp_vec.push_back( tmp_str );
        z_map[tmp_str] = tmp_vec;
    }
    
    // string k = "¤B";
    // for( vector<string>::iterator it = z_map[k].begin(); it != z_map[k].end(); ++it )
        // cout << *it << endl;

    // ifstream textfile("./testdata/1.txt");
    ifstream textfile( f_text.c_str() );
    if(textfile.is_open()){
        size_t pos = 0;
        while ( getline(textfile, line) ){
            // parse toks

            string delimiter = " ";
            string tok;
            vector<string> vec_sentence;
            while ((pos = line.find(delimiter)) != string::npos) {
                tok = line.substr(0, pos);
                // store to map
                // cout << "[" << tok << "]" << endl;
                if( tok != "" ) vec_sentence.insert( vec_sentence.end(), tok );
                line.erase(0, pos + delimiter.length());
            }
            tok = line.substr(0, line.find("\n"));
            if( tok != "" ) vec_sentence.insert( vec_sentence.end(), tok );
            vec_sentence.push_back( "</s>" );
            // for( vector<string>::iterator it_sen = vec_sentence.begin(); it_sen != vec_sentence.end(); ++it_sen ){
            //     cout << (*it_sen) << " ";
            // }
            // break;

            vector< vector<DPNode> > vec_DP;
            // vector< vector<DPNode> >::iterator it_DP;
            int idx_col = 0;

            for( vector<string>::iterator it_sen = vec_sentence.begin(); it_sen != vec_sentence.end(); ++it_sen ){
                vector<DPNode> vec_col;
                // cout << '\n' << (*it_sen) << '\t';

                for( vector<string>::iterator it_map = z_map[*it_sen].begin(); it_map != z_map[*it_sen].end(); ++it_map ){
                    // cout << (*it_map);
                    // continue;

                    // cout << "b";
                    DPNode nd_tmp;
                    if( it_sen == vec_sentence.begin() ){
                        nd_tmp.prob = getBigramProb("<s>", (*it_map).c_str(), voc, lm);
                        nd_tmp.prev = NULL;
                        nd_tmp.word = *it_map;
                        nd_tmp.sentence = *it_map;
                        // cout << "prob: " << nd_tmp.prob << " word: " << nd_tmp.word << endl;
                    }else{
                        double max_prob = 123123;
                        DPNode *max_node;

                        // for( vector<DPNode>::iterator it_prevnode = vec_DP[idx_col].begin(); it_prevnode != vec_DP[idx_col].end(); ++it_prevnode ){
                        for( int i=0; i<vec_DP[idx_col].size(); i++ ){
                            DPNode tmp = vec_DP[idx_col][i];
                            double tmp_prob = tmp.prob + getBigramProb( tmp.word.c_str(), (*it_map).c_str(), voc, lm );
                            if (max_prob == 123123 || tmp_prob > max_prob){
                                max_node = &vec_DP[idx_col][i];
                                max_prob = tmp_prob;
                            }
                        }

                        nd_tmp.prob = max_prob;
                        nd_tmp.prev = max_node;
                        nd_tmp.sentence = (*nd_tmp.prev).sentence + ' ' + (*it_map);
                        nd_tmp.word = (*it_map);
                        // cout << "prob: " << nd_tmp.prob << " word: " << (*nd_tmp.prev).word << nd_tmp.word << endl;
                    }
                    vec_col.push_back( nd_tmp );
                }

                vec_DP.push_back( vec_col );
                
                if( it_sen != vec_sentence.begin() ) 
                    ++idx_col;
            }

            DPNode max_node;
            DPNode *node_ptr;
            double max_prob = -999999.0;
            for( vector<DPNode>::iterator it = vec_DP[idx_col].begin(); it != vec_DP[idx_col].end(); ++it ){
                DPNode tmp = (*it);
                if(max_prob < tmp.prob){
                    max_prob = tmp.prob;
                    max_node = tmp;
                }
            }
            // cout << max_node.word << endl;
            cout << "<s> " << max_node.sentence << endl;

        }      
    }else cout << "Unable to open file"; 


}

