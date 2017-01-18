/* PSO encryption/decryption and compression functions. */

void start_encryption (PSO_CLIENT* connect);
static void pso_crypt_init_key_bb (unsigned char *data);
void pso_crypt_decrypt_bb (PSO_CRYPT *pcry, unsigned char *data, unsigned int length);
void pso_crypt_encrypt_bb (PSO_CRYPT *pcry, unsigned char *data, unsigned int length);
void encryptcopy (PSO_CLIENT* client, const unsigned char* src, unsigned int size);
void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned int size);
void pso_crypt_table_init_bb (PSO_CRYPT *pcry, const unsigned char *salt);
unsigned RleEncode (unsigned char *src, unsigned char *dest, unsigned src_size);
void RleDecode (unsigned char *src, unsigned char *dest, unsigned src_size);
void prepare_key (unsigned char *keydata, unsigned int len, struct rc4_key *key);
void rc4 (unsigned char *buffer, unsigned int len, struct rc4_key *key);
void compressShipPacket ( PSO_SERVER* ship, unsigned char* src, unsigned long src_size );
void decompressShipPacket ( PSO_SERVER* ship, unsigned char* dest, unsigned char* src );
