/* PSO encryption/decryption and compression functions. */

void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned int size);
unsigned RleEncode (unsigned char *src, unsigned char *dest, unsigned src_size);
void RleDecode (unsigned char *src, unsigned char *dest, unsigned src_size);
void prepare_key (unsigned char *keydata, unsigned int len, struct rc4_key *key);
void rc4 (unsigned char *buffer, unsigned int len, struct rc4_key *key);
