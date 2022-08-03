// SPDX-License-Identifier: MIT

#if defined(_WIN32)
#pragma warning(disable : 4244 4293)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oqs/oqs.h>

#include "system_info.c"
#include "tmp_store.c"

typedef enum sig_ops {
	SIG_KEYGEN      = 0,
	SIG_STFL_SIGN   = 1,
	SIG_STFL_VERIFY = 2
} SIG_OPS;

OQS_STATUS lock_sk_key(OQS_SECRET_KEY *sk) {
	printf("%02x", sk->secret_key[0]);
	return 0;
}

OQS_STATUS release_sk_key(OQS_SECRET_KEY *sk) {
	printf("%02x", sk->secret_key[0]);
	return 0;
}

OQS_STATUS do_nothing_save(const OQS_SECRET_KEY *sk) {
	printf("%02x", sk->secret_key[0]);
	return 0;
}

static OQS_STATUS sig_test_correctness(const char *method_name, SIG_OPS op) {

	OQS_SIG_STFL *sig = NULL;
	uint8_t *public_key = NULL;
	OQS_SECRET_KEY *secret_key = NULL;
	uint8_t *message = NULL;
	size_t message_len = 100;
	uint8_t *signature = NULL;
	size_t signature_len;
	OQS_STATUS rc, ret = OQS_ERROR;

	sig = OQS_SIG_STFL_new(method_name);
	if (sig == NULL) {
		fprintf(stderr, "ERROR: OQS_SIG_STFL_new failed\n");
		goto err;
	}

	switch (op) {
	case SIG_KEYGEN:
		printf("================================================================================\n");
		printf("Executing keygen for SIG_STFL_ALG %s\n", sig->method_name);
		printf("================================================================================\n");

		public_key = malloc(sig->length_public_key);

		secret_key = OQS_SECRET_KEY_new(method_name);
		secret_key->lock_key = lock_sk_key;
		secret_key->release_key = release_sk_key;
		secret_key->oqs_save_updated_sk_key = do_nothing_save;

		if ((public_key == NULL) || (secret_key == NULL)) {
			fprintf(stderr, "ERROR: malloc failed\n");
			goto err;
		}
		rc = OQS_SIG_STFL_keypair(sig, public_key, secret_key);
		if (rc != OQS_SUCCESS) {
			fprintf(stderr, "ERROR: OQS_SIG_STFL_KEYPAIR failed\n");
			goto err;
		}
		if (oqs_fstore("pk", method_name, public_key, sig->length_public_key) != OQS_SUCCESS) {
			goto err;
		}
		if (oqs_fstore("sk", method_name, secret_key->secret_key, secret_key->length_secret_key) != OQS_SUCCESS) {
			goto err;
		}
		ret = OQS_SUCCESS;
		goto cleanup;

	case SIG_STFL_SIGN:
		printf("================================================================================\n");
		printf("Executing sign for SIG_STFL_ALG %s\n", sig->method_name);
		printf("================================================================================\n");

		public_key = malloc(sig->length_public_key);
		secret_key = OQS_SECRET_KEY_new(method_name);
		secret_key->lock_key = lock_sk_key;
		secret_key->release_key = release_sk_key;
		secret_key->oqs_save_updated_sk_key = do_nothing_save;
		message = malloc(message_len);
		signature = malloc(sig->length_signature);

		if ((public_key == NULL) || (secret_key == NULL) || (message == NULL) || (signature == NULL)) {
			fprintf(stderr, "ERROR: malloc failed\n");
			goto err;
		}
		if (oqs_fload("pk", method_name, public_key, sig->length_public_key, &signature_len) != OQS_SUCCESS) {
			goto err;
		}
		if (oqs_fload("sk", method_name, secret_key->secret_key, secret_key->length_secret_key, &signature_len) != OQS_SUCCESS) {
			goto err;
		}

		OQS_randombytes(message, message_len);

		rc = OQS_SIG_STFL_sign(sig, signature, &signature_len, message, message_len, secret_key);
		if (rc != OQS_SUCCESS) {
			fprintf(stderr, "ERROR: OQS_SIG_STFL_SIGN failed\n");
			goto err;
		}
		if (oqs_fstore("ct", method_name, message, message_len) != OQS_SUCCESS) {
			goto err;
		}
		if (oqs_fstore("se", method_name, signature, signature_len) != OQS_SUCCESS) {
			goto err;
		}
		ret = OQS_SUCCESS;
		goto cleanup;

	case SIG_STFL_VERIFY:
		printf("================================================================================\n");
		printf("Executing verify for SIG_STFL_ALG %s\n", sig->method_name);
		printf("================================================================================\n");

		public_key = malloc(sig->length_public_key);
		secret_key = OQS_SECRET_KEY_new(method_name);
		secret_key->lock_key = lock_sk_key;
		secret_key->release_key = release_sk_key;
		secret_key->oqs_save_updated_sk_key = do_nothing_save;
		message = malloc(message_len);
		signature = malloc(sig->length_signature);

		if ((public_key == NULL) || (secret_key == NULL) || (message == NULL) || (signature == NULL)) {
			fprintf(stderr, "ERROR: malloc failed\n");
			goto err;
		}
		if (oqs_fload("pk", method_name, public_key, sig->length_public_key, &signature_len) != OQS_SUCCESS) {
			goto err;
		}
		if (oqs_fload("sk", method_name, secret_key->secret_key, secret_key->length_secret_key, &signature_len) != OQS_SUCCESS) {
			goto err;
		}
		if (oqs_fload("ct", method_name, message, message_len, &signature_len) != OQS_SUCCESS) {
			goto err;
		}
		if (oqs_fload("se", method_name, signature, sig->length_signature, &signature_len) != OQS_SUCCESS) {
			goto err;
		}

		rc = OQS_SIG_STFL_verify(sig, message, message_len, signature, signature_len, public_key);
		if (rc != OQS_SUCCESS) {
			fprintf(stderr, "ERROR: OQS_SIG_STFL_VERify failed\n");
			goto err;
		}

		printf("verification passes as expected\n");
		ret = OQS_SUCCESS;
		goto cleanup;

	default:
		fprintf(stderr, "Incorrect operation requested. Aborting.\n");
		goto err;
	}

err:
	ret = OQS_ERROR;

cleanup:
	if (public_key) {
		OQS_MEM_insecure_free(public_key);
	}
	if (message) {
		OQS_MEM_insecure_free(message);
	}
	if (signature) {
		OQS_MEM_insecure_free(signature);
	}
	if (sig != NULL) {
		OQS_SECRET_KEY_free(secret_key);
		OQS_SIG_STFL_free(sig);
	}

	return ret;
}

int main(int argc, char **argv) {

	if (argc != 3) {
		fprintf(stderr, "Usage: test_sig algname operation (0,1,2)\n");
		fprintf(stderr, "  algname: ");
		for (size_t i = 0; i < OQS_SIG_STFL_algs_length; i++) {
			if (i > 0) {
				fprintf(stderr, ", ");
			}
			fprintf(stderr, "%s", OQS_SIG_STFL_alg_identifier(i));
		}
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}

	print_system_info();

	char *alg_name = argv[1];
	if (!OQS_SIG_STFL_alg_is_enabled(alg_name)) {
		printf("Signature algorithm %s not enabled!\n", alg_name);
		return EXIT_FAILURE;
	}

	// Use system RNG in this program
	OQS_randombytes_switch_algorithm(OQS_RAND_alg_system);

	oqs_fstore_init();

	OQS_STATUS rc = sig_test_correctness(alg_name, (unsigned int)atoi(argv[2]));

	if (rc != OQS_SUCCESS) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}