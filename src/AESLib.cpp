#include "AESLib.h"

uint8_t AESLib::getrnd()
{
   return aes.getrandom();
}

void AESLib::gen_iv(byte  *iv) {
    for (int i = 0 ; i < N_BLOCK ; i++ ) {
        iv[i]= (byte)getrnd();
    }
}

void AESLib::set_paddingmode(paddingMode mode){
  aes.setPadMode(mode);
}

paddingMode AESLib::get_paddingmode(){
  return aes.getPadMode();
}


int AESLib::get_cipher_length(int msglen){
  return aes.get_padded_len(msglen);
}


int AESLib::get_cipher64_length(int msglen){
  return base64_enc_len(aes.get_padded_len(base64_enc_len(msglen)));
}

/* Returns Arduino String decoded and decrypted. */
String AESLib::decrypt(String msg, byte key[],int bits, byte my_iv[]) {

  aes.set_key(key, bits);

  int len = msg.length();
  char encrypted[base64_dec_len((char*)msg.c_str(),len)];
  int b64len = base64_decode(encrypted, (char*)msg.c_str(), msg.length());

  byte out[b64len];
  int plain_len = aes.do_aes_decrypt((byte *)encrypted, b64len, out, key, bits, (byte *)my_iv);
  // unpad the string
  out[plain_len] = 0; // add string termination

  int outLen = base64_dec_len((char*)out, plain_len);
  char message[outLen+1]; // trailing zero for cstring

  outLen = base64_decode(message, (char *)out, plain_len);
  //message[baseLen] = '\0'; // ensure trailing zero after cstring <--not needed is already done in base64_decode

  return String(message);
}

/* Returns byte array decoded and decrypted. */
void AESLib::decrypt(char * msg, char * plain, byte key[],int bits, byte my_iv[]) {

  aes.set_key(key, bits);

  int msgLen = strlen(msg);
  char encrypted[msgLen]; // will be always shorter than Base64
  int b64len = base64_decode(encrypted, msg, msgLen);

  byte out[2*msgLen];
  int plain_len = aes.do_aes_decrypt((byte *)encrypted, b64len, out, key, bits, (byte *)my_iv);
  // unpad the string
  out[plain_len ] = 0; // add string termination

  int outLen = base64_dec_len((char*)out, plain_len);
  char message[outLen+1]; // trailing zero for cstring?

  strcpy(plain, message);
}

/* Returns Arduino string encrypted and encoded with Base64. */
String AESLib::encrypt(String msg, byte key[],int bits, byte my_iv[]) {

  aes.set_key(key, bits);

  int msgLen = strlen(msg.c_str());

  char b64data[base64_enc_len(msgLen)+1]; //+1 to store \0 at the end
  int b64len = base64_encode(b64data, (char*)msg.c_str(), msgLen);

  // paddedLen is a mutiple of the N_BLOCK size
  // paddedLen = (((int)(b64len/N_BLOCK) + 1)*N_BLOCK
  // KOV the +1 is not needed since this is no string anymore so no \0 character needed for ending
  //     depending on the padding strategy an additional N_BLOCK bytes will be added
  int paddedLen =  aes.get_padded_len(b64len);
  byte padded[paddedLen];
  aes.padPlaintext(b64data, padded);

  // cipher will keep the length of the padded message
  // do_aes_encrypt will pad the message so use the unpadded source
  byte cipher[paddedLen];
  aes.do_aes_encrypt((byte *)b64data, b64len, cipher, key, bits, my_iv);

  char out[base64_enc_len(paddedLen)+1];
  base64_encode(out, (char *)cipher, paddedLen );

  return String((char*)out);
}

/* Returns message encrypted and base64 encoded to be used as string. */
uint16_t AESLib::encrypt64(char * msg, char * output, byte key[],int bits, byte my_iv[]) {

  aes.set_key(key, bits);

  int msgLen = strlen(msg);
  char b64data[base64_enc_len(msgLen)+1];  // should add 1 character to accomodate the 0x\0 ending character

  int b64len = base64_encode(b64data, msg, msgLen);
  int paddedLen = aes.get_padded_len(b64len);

  byte cipher[paddedLen];
  aes.do_aes_encrypt((byte *)b64data, b64len, cipher, key, bits, my_iv);

  uint16_t encrypted_length = aes.get_size();
  base64_encode(output, (char *)cipher, aes.get_size() );

  return encrypted_length;
}

/* Suggested size for the plaintext buffer is 1/2 length of `msg` */
uint16_t AESLib::decrypt64(char * msg, char * plain, byte key[],int bits, byte my_iv[]) {

  aes.set_key(key, bits);

  int msgLen = strlen(msg);
  char encrypted[base64_dec_len(msg,msgLen)+1];
  int b64len = base64_decode(encrypted, msg, msgLen);
  // decrypting will keep the message length
  byte out[b64len];
  memset(out, 0, sizeof(out));
  int plain_len = aes.do_aes_decrypt((byte *)encrypted, b64len, out, key, bits, (byte *)my_iv);
  out[plain_len] = 0; // ensure string termination

#ifdef AES_DEBUG
  Serial.print("[decrypt64] plain length:  "); Serial.println(plain_len);
#endif

  // calculate required output length
  uint16_t outLen = base64_dec_len((char*)out, plain_len);

#ifdef AES_DEBUG
  Serial.print("[decrypt64] base64_dec_len:  "); Serial.println(outLen);
#endif

  outLen = base64_decode(plain, (char *)out, plain_len);
  plain[outLen+1] = 0; // trailing zero

#ifdef AES_DEBUG
  Serial.print("[decrypt64] base64_decode->outLen =  "); Serial.println(outLen);
#endif

  return outLen;
}

/* Returns message encrypted only to be used as byte array. */
void AESLib::encrypt(char * msg, char * output, byte key[],int bits, byte my_iv[]) {

  aes.set_key(key, bits);

  int msgLen = strlen(msg);

  char b64data[base64_enc_len(msgLen)+1];
  int b64len = base64_encode(b64data, (char*)msg, msgLen);

  int paddedLen =  aes.get_padded_len(b64len);;
  byte padded[paddedLen];
  aes.padPlaintext(b64data, padded);
  // encryption will return the same number of bytes as the padded message
  // do_aes_encrypt will pad the message so use the unpadded source
  byte cipher[paddedLen];
  aes.do_aes_encrypt((byte *)b64data, b64len, cipher, key, bits, my_iv);

  strcpy(output, (char*)cipher);
}

void AESLib::clean() {
  aes.clean();
}
