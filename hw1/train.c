#include "hmm.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// no magic num
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

void print_params( const PARAMS p );

void get_alpha( PARAMS *alpha, const char *seq, const HMM hmm ){
	
	for(int i=0; i < hmm.state_num; i++){
		alpha->val[0][i] = hmm.initial[i] * hmm.observation[seq[0] - ASCII_A][i];
		// printf("%f\t%f\t%f\n", hmm.initial[i], hmm.observation[line[0] - ASCII_A][i], alpha[0][i]);
	}
	
	double acc_tmp;
	for(int t=1; t < alpha->len_seq; t++){
		//printf("%d", t);
		for(int j=0; j<hmm.state_num; j++){
			acc_tmp = 0.0;
			for(int i=0; i<hmm.state_num; i++){
				acc_tmp += alpha->val[t-1][i] * hmm.transition[i][j];
				/*
				if( t == alpha->len_seq -1 )
					printf("alpha[%d][%d]:%E * hmm_trans[%d][%d]:%E -> %E, acc: %E\n", t-1, i, alpha->val[t-1][i], i, j, hmm.transition[i][j], alpha->val[t-1][i]*hmm.transition[i][j], acc_tmp);
				*/
			}
			//printf("acc:%E * obs[%d][%d]:%E --> %E", acc_tmp, seq[t]-ASCII_A, j, hmm.observation[seq[t] - ASCII_A][j], acc_tmp*hmm.observation[seq[t] - ASCII_A][j]);
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

void print_params( const PARAMS p ){
	
	printf("state_num: %d, seq_len: %d\n", p.state_num, p.len_seq);	
	for(int i=0; i < p.state_num; i++){
		for(int j=0; j < p.len_seq; j++){
			printf("%E ", p.val[j][i]);
		}
		printf("\n");
	}
	return;
}

int main(int argc, char **argv){

	int iterations;
	char init_model[40], training_seq[40], output_name[40];

	if( argc != 5 ){
		printf("Usage: train iterations init_model training_sequence output_name\n");
		exit(-1);
	}else{
		iterations = atoi( argv[1] );
		strcpy( init_model, argv[2] );
		strcpy( training_seq, argv[3] );
		strcpy( output_name, argv[4] );
	}
	
	/*
	iterations = 50;
	strcpy( init_model, "model_init.txt");
	strcpy( training_seq, "seq_model_01.txt");
	strcpy( output_name, "model_01.txt");
	*/

	//	printf( "iter=%d, init=%s, training=%s, output=%s\n", iterations, init_model, training_seq, output_name );

	HMM hmm_init;
	loadHMM( &hmm_init, init_model );

	// init update accs
	double pi_deno[MAX_STATE];
	double pi_nume[MAX_STATE];
	double a_deno[MAX_STATE][MAX_STATE];
	double a_nume[MAX_STATE][MAX_STATE];
	double b_deno[MAX_OBSERV][MAX_STATE];
	double b_nume[MAX_OBSERV][MAX_STATE];


	for(int iter=0; iter<iterations; iter++){
		
		for(int i=0; i<MAX_STATE; i++){
			pi_deno[i] = 0.0;
			pi_nume[i] = 0.0;
			for(int j=0; j<MAX_STATE; j++){
				a_nume[i][j] = 0.0;
				a_deno[i][j] = 0.0;
			}
			for(int j=0; j<MAX_OBSERV; j++){
				b_nume[j][i] = 0.0;
				b_deno[j][i] = 0.0;
			}
		}
		
		FILE * fp_seq;
		char * line = NULL;
		size_t len = 0;
		ssize_t len_seq;

		fp_seq = fopen(training_seq, "r");
		if (fp_seq == NULL)
			exit(-1);
		
		int counter = 0;
		//printf("iter:%d\n", iter);
		while ( (len_seq = getline(&line, &len, fp_seq)) != -1) {
			--len_seq; // remove '\n'

			PARAMS gemma;
			gemma.state_num = hmm_init.state_num;
			gemma.len_seq = len_seq;

			EPSILON epsilon;
			epsilon.state_num = hmm_init.state_num;
			epsilon.len_seq = len_seq;
			
			get_params( &gemma, &epsilon, line, hmm_init );

			for(int i=0; i<hmm_init.state_num; i++){
				pi_deno[i] += 1.0;
				pi_nume[i] += gemma.val[0][i];
				
				for(int j=0; j<hmm_init.state_num; j++){
					
					for(int t=0; t<len_seq-1; t++){
						a_nume[i][j] += epsilon.val[t][i][j];
						a_deno[i][j] += gemma.val[t][i];
					}
				}

				for(int k=0; k<hmm_init.observ_num; k++){
					for(int t=0; t<len_seq; t++){
						b_deno[k][i] += gemma.val[t][i];
						b_nume[k][i] += ( (line[t] - ASCII_A) == k )? gemma.val[t][i] : 0;
					}
				}	
			}

		}

		fclose(fp_seq);
		if (line)
			free(line);

		// update hmm
		for(int i=0; i<hmm_init.state_num; i++){
			hmm_init.initial[i] = pi_nume[i] / pi_deno[i];
			for(int j=0; j<hmm_init.state_num; j++)
				hmm_init.transition[i][j] = a_nume[i][j] / a_deno[i][j];
			for(int k=0; k<hmm_init.observ_num; k++)
				hmm_init.observation[k][i] = b_nume[k][i] / b_deno[k][i];
		}
	}

	FILE *fp_dump;
	fp_dump = fopen( output_name, "w" );
	if( fp_dump == NULL )
		exit(-1);
	dumpHMM( fp_dump, &hmm_init );

	return 0;
}






