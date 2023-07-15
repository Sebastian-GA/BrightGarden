#include <AESLib.h>

AESLib aesLib;

// AES Encryption Key
byte aes_key[] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_g_iv[N_BLOCK] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // This is not a constant

const byte default_aes_iv[N_BLOCK] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
byte aes_iv[N_BLOCK];

void aes_reset_iv() {
    for (int i = 0; i < 16; i++)
        aes_iv[i] = default_aes_iv[i];
}

String aes_encrypt(char *msg) {
    aes_reset_iv();

    unsigned int msgLen = strlen(msg);
    unsigned int cipherlength = aesLib.get_cipher64_length(msgLen);
    char encrypted[cipherlength] = {0};  // AHA! needs to be large, 2x is not enough
    // char encrypted[2*msgLen] = {0};

    aesLib.encrypt64((const byte *)msg, msgLen, encrypted, aes_key, sizeof(aes_key), aes_iv);
    return String(encrypted);
}

String aes_decrypt(char *msg) {
    aes_reset_iv();

    unsigned int msgLen = strlen(msg);
    char decrypted[msgLen] = {0};  // Half size could cause errors // TODO: optimize this size

    aesLib.decrypt64(msg, msgLen, (byte *)decrypted, aes_key, sizeof(aes_key), aes_iv);

    // Remove padding

    unsigned int cipherlength = 0;
    byte padding_length = 0x00;
    for (int i = msgLen - 1; i >= 0; i--) {
        if (decrypted[i] != 0x00) {
            cipherlength = i + 1;
            padding_length = decrypted[i];
            break;
        }
    }
    bool is_valid_padding = true;
    for (int i = cipherlength - padding_length; i < cipherlength - 1; i++) {
        if (decrypted[i] != padding_length) {
            is_valid_padding = false;
            break;
        }
    }
    if (is_valid_padding) {
        decrypted[cipherlength - padding_length] = 0x00;  // It's only necceesary to clear this byte
    }

    return String(decrypted);
}

// Generate IV (once)
void aes_init() {
    aesLib.gen_iv(aes_g_iv);
    aesLib.set_paddingmode(paddingMode::CMS);

    char b64in[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    char b64out[base64_enc_len(sizeof(aes_g_iv))];
    base64_encode(b64out, b64in, 16);

    char b64enc[base64_enc_len(10)];
    base64_encode(b64enc, (char *)"0123456789", 10);

    char b64dec[base64_dec_len(b64enc, sizeof(b64enc))];
    base64_decode(b64dec, b64enc, sizeof(b64enc));

    aes_reset_iv();
}

/* non-blocking wait function */
void wait(unsigned long milliseconds) {
    unsigned long timeout = millis() + milliseconds;
    while (millis() < timeout) {
        yield();
    }
}
