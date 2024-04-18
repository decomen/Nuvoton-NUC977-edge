
#include "mdtypedef.h"
#include "dm_md5.h"

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static const mdBYTE PADDING[64] =
{
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
 
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
 
#define FF(a, b, c, d, x, s, ac)          \
{                       \
    (a) += F((b), (c), (d)) + (x) + (mdUINT32)(ac);  \
    (a) = ROTATE_LEFT((a), (s));           \
    (a) += (b);                 \
}
 
#define GG(a, b, c, d, x, s, ac)          \
{                       \
    (a) += G((b), (c), (d)) + (x) + (mdUINT32)(ac);  \
    (a) = ROTATE_LEFT((a), (s));           \
    (a) += (b);                 \
}
 
#define HH(a, b, c, d, x, s, ac) \
{ \
    (a) += H((b), (c), (d)) + (x) + (mdUINT32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
 
#define II(a, b, c, d, x, s, ac)          \
{                       \
    (a) += I((b), (c), (d)) + (x) + (mdUINT32)(ac);  \
    (a) = ROTATE_LEFT((a), (s));           \
    (a) += (b);                 \
}

static void DM_MD5_Encode(mdBYTE * output, mdUINT32 * input, mdINT len)
{
    mdUINT32 i, j;
 
    for (i = 0, j = 0; j < len; i++, j += 4)
    {
       output[j] = (mdBYTE) (input[i] & 0xff);
       output[j + 1] = (mdBYTE) ((input[i] >> 8) & 0xff);
       output[j + 2] = (mdBYTE) ((input[i] >> 16) & 0xff);
       output[j + 3] = (mdBYTE) ((input[i] >> 24) & 0xff);
    }
}

static void DM_MD5_Decode(mdUINT32 * output, mdBYTE * input, mdINT len)
{
    mdUINT32 i, j;
 
    for (i = 0, j = 0; j < len; i++, j += 4)
    {
       output[i] = ((mdUINT32) input[j]) |
           (((mdUINT32) input[j + 1]) << 8) |
           (((mdUINT32) input[j + 2]) << 16) |
           (((mdUINT32) input[j + 3]) << 24);
    }
}

static void DM_MD5_Transform(mdUINT32 state[4], mdBYTE block[64])
{
    mdUINT32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
 
    DM_MD5_Decode(x, block, 64);
 
    /* Round 1 */
    FF(a, b, c, d, x[0], S11, 0xd76aa478);    /* 1 */
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);    /* 2 */
    FF(c, d, a, b, x[2], S13, 0x242070db);    /* 3 */
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);    /* 4 */
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);    /* 5 */
    FF(d, a, b, c, x[5], S12, 0x4787c62a);    /* 6 */
    FF(c, d, a, b, x[6], S13, 0xa8304613);    /* 7 */
    FF(b, c, d, a, x[7], S14, 0xfd469501);    /* 8 */
    FF(a, b, c, d, x[8], S11, 0x698098d8);    /* 9 */
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);    /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);   /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be);   /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122);   /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193);   /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e);   /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821);   /* 16 */
 
    /* Round 2 */
    GG(a, b, c, d, x[1], S21, 0xf61e2562);    /* 17 */
    GG(d, a, b, c, x[6], S22, 0xc040b340);    /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51);   /* 19 */
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);    /* 20 */
    GG(a, b, c, d, x[5], S21, 0xd62f105d);    /* 21 */
    GG(d, a, b, c, x[10], S22, 0x2441453);    /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);   /* 23 */
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);    /* 24 */
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);    /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6);   /* 26 */
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);    /* 27 */
    GG(b, c, d, a, x[8], S24, 0x455a14ed);    /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);   /* 29 */
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);    /* 30 */
    GG(c, d, a, b, x[7], S23, 0x676f02d9);    /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);   /* 32 */
 
    /* Round 3 */
    HH(a, b, c, d, x[5], S31, 0xfffa3942);    /* 33 */
    HH(d, a, b, c, x[8], S32, 0x8771f681);    /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);   /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c);   /* 36 */
    HH(a, b, c, d, x[1], S31, 0xa4beea44);    /* 37 */
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);    /* 38 */
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);    /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);   /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);   /* 41 */
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);    /* 42 */
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);    /* 43 */
    HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);    /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);   /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);   /* 47 */
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);    /* 48 */
 
    /* Round 4 */
    II(a, b, c, d, x[0], S41, 0xf4292244);    /* 49 */
    II(d, a, b, c, x[7], S42, 0x432aff97);    /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7);   /* 51 */
    II(b, c, d, a, x[5], S44, 0xfc93a039);    /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3);   /* 53 */
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);    /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d);   /* 55 */
    II(b, c, d, a, x[1], S44, 0x85845dd1);    /* 56 */
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);    /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);   /* 58 */
    II(c, d, a, b, x[6], S43, 0xa3014314);    /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1);   /* 60 */
    II(a, b, c, d, x[4], S41, 0xf7537e82);    /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235);   /* 62 */
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);    /* 63 */
    II(b, c, d, a, x[9], S44, 0xeb86d391);    /* 64 */
 
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
 
    memset((mdBYTE *) x, 0, sizeof(x));
}
 
void DM_MD5_Init(DM_MD5Context * context)
{
    context->count[0] = context->count[1] = 0;

    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

void DM_MD5_Update(DM_MD5Context * context, mdBYTE * buf, mdINT len)
{
    mdUINT32 i, index, partLen;

    index = (mdUINT32) ((context->count[0] >> 3) & 0x3F);

    if ((context->count[0] += ((mdUINT32) len << 3)) < ((mdUINT32) len << 3)) {
        context->count[1]++;
    }
    
    context->count[1] += ((mdUINT32) len >> 29);

    partLen = 64 - index;

    if (len >= partLen) {
        memcpy((mdBYTE *) &context->buffer[index], (mdBYTE *) buf, partLen);
        DM_MD5_Transform(context->state, context->buffer);

        for (i = partLen; i + 63 < len; i += 64) {
            DM_MD5_Transform(context->state, &buf[i]);
        }

        index = 0;
    } else {
        i = 0;
    }

    memcpy((mdBYTE *) &context->buffer[index], (mdBYTE *) &buf[i], len - i);
}

void DM_MD5_Final(DM_MD5Context * context, mdBYTE digest[16])
{
    mdBYTE bits[8];
    mdUINT32 index, padLen;

    DM_MD5_Encode(bits, context->count, 8);

    index = (mdUINT32) ((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    DM_MD5_Update(context, (mdBYTE *)PADDING, padLen);

    DM_MD5_Update(context, bits, 8);

    DM_MD5_Encode(digest, context->state, 16);

    memset((mdBYTE *) context, 0, sizeof(*context));
}

void DM_MD5_Final_16(DM_MD5Context * context, mdBYTE digest[8])
{
	mdBYTE tmpdig[16];
	
	DM_MD5_Final(context, tmpdig);
	memcpy((void*)digest, (mdBYTE*)tmpdig+4, 8);
}


