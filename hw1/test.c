#include "hmm.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MODEL 5
#define ASCII_A 65

typedef struct{
	double val[MAX_SEQ][MAX_STATE];
	int len_seq;
	int state_num;
} PARAMS;

typedef struct{
	double val[MAX_SEQ][MAX_STATE][MAX_STATE];
	int len_seq;
	int state_num;
} EPSILON;

void get_alpha( PARAMS *alpha, const char *seq, const HMM hmm ){
	
	for(int i=0; i < hmm.state_num; i++){
		alpha->val[0][i] = hmm.initial[i] * hmm.observation[seq[0] - ASCII_A][i];
	}
	
	double acc_tmp;
	for(int t=1; t < alpha->len_seq; t++){
		for(int j=0; j<hmm.state_num; j++){
			acc_tmp = 0.0;
			for(int i=0; i<hmm.state_num; i++){
				acc_tmp += alpha->val[t-1][i] * hmm.transition[i][j];
			}
			alpha->val[t][j] = acc_tmp * hmm.observation[seq[t] - ASCII_A][j];
		}
	}
	return; 
}

void get_beta( PARAMS *beta, const char *seq, const HMM hmm){
	
	for(int i=0; i<hmm.state_num; i++){
		beta->val[ beta->len_seq - 1 ][i] = 1.0;
	}
	
	// print_params( *beta );

	for(int t = beta->len_seq - 2; t >= 0; t--){
		for(int i=0; i<hmm.state_num; i++){
			double acc_tmp = 0.0;
			for(int j=0; j<hmm.state_num; j++){
				acc_tmp += beta->val[t+1][j] * hmm.transition[i][j] * hmm.observation[ seq[t+1] - ASCII_A ][j];
			}
			beta->val[t][i] = acc_tmp;
		}
	}

	return;
}

void get_params( PARAMS *gemma, EPSILON *epsilon, const char *seq, const HMM hmm ){
	
	PARAMS alpha, beta;
	
	alpha.len_seq = gemma->len_seq;
	beta.len_seq = gemma->len_seq;
	alpha.state_num = gemma->state_num;
	beta.state_num = gemma->state_num;
	
	get_alpha( &alpha, seq, hmm);
	get_beta( &beta, seq, hmm);
	
	// print_params( alpha );
	// print_params( beta );	

	for(int t=0; t<gemma->len_seq; t++){
		double tmp_acc = 0.0;
		for(int i=0; i<gemma->state_num; i++){
			gemma->val[t][i] = alpha.val[t][i] * beta.val[t][i];
			tmp_acc += gemma->val[t][i];
			// if( t == gemma->len_seq -1 ) printf("%f %f ->> %f\n", alpha.val[t][i], beta.val[t][i], tmp_acc);
		}
		for(int i=0; i<gemma->state_num; i++)
			gemma->val[t][i] /= tmp_acc;
	}

	for(int t=0; t < epsilon->len_seq - 1; t++){
		double acc_tmp = 0.0;
		for(int i=0; i < epsilon->state_num; i++){
			for(int j=0; j < epsilon->state_num; j++){
				epsilon->val[t][i][j] = alpha.val[t][i] * hmm.transition[i][j] * hmm.observation[ seq[t+1] - ASCII_A ][j] * beta.val[t+1][j];
				acc_tmp += epsilon->val[t][i][j];
			}
		}
		//printf("epsilon acc: %E\n", acc_tmp);
		for(int i=0; i < epsilon->state_num; i++){
			for(int j=0; j < epsilon->state_num; j++){
				epsilon->val[t][i][j] /= acc_tmp;
			}
		}
		
	}

}

void get_delta( PARAMS *delta, const char *seq, const HMM hmm){
	
	int len_seq = strlen(seq);
	
	for(int i=0; i<hmm.state_num; i++)
		delta->val[0][i] = hmm.initial[i] * hmm.observation[ seq[0] - ASCII_A ][i];
	
	for( int t=1; t<len_seq; t++){
		for( int j=0; j<hmm.state_num; j++){
			double max_prob = 0.0;
			for( int i=0; i<hmm.state_num; i++){
				double prob = delta->val[t-1][i] * hmm.transition[i][j];
				if( prob > max_prob )
					max_prob = prob;
			}
			delta->val[t][j] = max_prob * hmm.observation[ seq[t] - ASCII_A ][j];
		}
	}
	
	return;
}

double get_prob( const HMM hmm, const char *seq ){
	
	int len_seq = strlen( seq );
	if(len_seq != 50){
		printf("len_seq: %d", len_seq);
		exit(-1);
	}

	/*
	PARAMS alpha, beta;
	alpha.state_num = hmm.state_num;
	beta.state_num = hmm.state_num;
	alpha.len_seq = len_seq;
	beta.len_seq = len_seq;

	get_alpha( &alpha, seq, hmm);
	get_beta( &beta, seq, hmm);
	
	double prob1 = 0.0;
	for(int i; i<hmm.state_num; i++)
		prob1 += alpha.val[len_seq-1][i];

	double prob2 = 0.0;
	for(int i; i<hmm.state_num; i++)
		prob2 += alpha.val[10][i] * beta.val[10][i];

	double prob3 = 0.0;
	for(int i; i<hmm.state_num; i++)
		prob3 += alpha.val[40][i] * beta.val[40][i];

	printf("%E, %E, %E\n", prob1, prob2, prob3);
	exit(0);
	*/

	PARAMS delta;
	delta.state_num = hmm.state_num;
	delta.len_seq = len_seq;
	get_delta( &delta, seq, hmm );
	
	double max_prob = 0.0;
	for( int i=0; i<hmm.state_num; i++){
		if( delta.val[len_seq-1][i] > max_prob )
			max_prob = delta.val[len_seq-1][i];
	}

	return max_prob;

	/*
	PARAMS alpha;
	alpha.state_num = hmm.state_num;
	alpha.len_seq = len_seq;
	get_alpha( &alpha, seq, hmm );
	double acc = 0.0;
	for( int i=0; i<hmm.state_num; i++)
		acc += alpha.val[len_seq - 1][i];
	return acc;
	*/
}

int main(int argc, char **argv){
	
	char model_list[40];
	char testing_data[40];
	char result[40];
	
	if( argc != 4 ){
		printf("Usage: test modellist testing_data result\n\n");
		exit(-1);
	}else{
		strcpy( model_list, argv[1] );
		strcpy( testing_data, argv[2] );
		strcpy( result, argv[3] );
	}

	HMM hmm[MAX_MODEL];

	load_models( model_list, hmm, MAX_MODEL );

	FILE * fp_testing;
	FILE * fp_result;
	char * line = NULL;
	size_t len = 0;
	ssize_t len_seq;


	fp_result = fopen( result, "w" );
	fp_testing = fopen( testing_data, "r");
	if (fp_testing == NULL || fp_result == NULL)
		exit(-1);
	
	int counter = 0;
	while ( (len_seq = getline(&line, &len, fp_testing)) != -1) {
		--len_seq; // remove '\n'
		line[len_seq] = '\0';
		double max_prob = 0.0;
		char max_name[40];
		for(int i=0; i<MAX_MODEL; i++){
			//printf("%s: %E\n", hmm[i].model_name, get_prob( hmm[i], line ) );
			double prob = get_prob( hmm[i], line );
			if(prob > max_prob){
				max_prob = prob;
				strcpy( max_name, hmm[i].model_name );
			}
		}
		fprintf( fp_result, "%s %E\n", max_name, max_prob );
	}

//	dump_models( hmm, MAX_MODEL );

	/*
	char *models[MAX_MODEL];
	for(int i=0; i<MAX_MODEL; i++)
		models[i] = (char*)malloc( 40 * sizeof(char) );

	size_t len=0;
	for(int i=0; i<MAX_MODEL; i++){
		getline( &models[i], &len, fp_list );
		models[i][ strlen(models[i]) - 1 ] = '\0';
		printf("%s\n", models[i]);
	}
	*/

	return 0;
}

