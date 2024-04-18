///////////////////////////////////////////////////////////////////////////////
// 文 件 名：AES.h
// 描    述：AES加密算法
// 创 建 人：Liangbofu
// 创建日期：2009-07-17
///////////////////////////////////////////////////////////////////////////////
#ifndef __AES_H
#define __AES_H

#ifdef __cplusplus
	extern "C" {
#endif

// 以bit为单位的密钥长度，只能为 128，192 和 256 三种
#define AES_KEY_LENGTH	128

// 加解密模式
#define AES_MODE_ECB	0				// 电子密码本模式（一般模式）
#define AES_MODE_CBC	1				// 密码分组链接模式
#define AES_MODE		AES_MODE_CBC


///////////////////////////////////////////////////////////////////////////////
//	函数名：	AES_Init
//	描述：		初始化，在此执行扩展密钥操作。
//	输入参数：	pKey -- 原始密钥，其长度必须为 AES_KEY_LENGTH/8 字节。
//	输出参数：	无。
//	返回值：	无。
///////////////////////////////////////////////////////////////////////////////
void AES_Init(const void *pKey);

//////////////////////////////////////////////////////////////////////////
//	函数名：	AES_Encrypt
//	描述：		加密数据
//	输入参数：	pPlainText	-- 明文，即需加密的数据，其长度为nDataLen字节。
//				nDataLen	-- 数据长度，以字节为单位，必须为AES_KEY_LENGTH/8的整倍数。
//				pIV			-- 初始化向量，如果使用ECB模式，可设为NULL。
//	输出参数：	pCipherText	-- 密文，即由明文加密后的数据，可以与pPlainText相同。
//	返回值：	无。
//////////////////////////////////////////////////////////////////////////
void AES_Encrypt(const unsigned char *pPlainText, unsigned char *pCipherText, 
				 unsigned int nDataLen, const unsigned char *pIV);

//////////////////////////////////////////////////////////////////////////
//	函数名：	AES_Decrypt
//	描述：		解密数据
//	输入参数：	pCipherText -- 密文，即需解密的数据，其长度为nDataLen字节。
//				nDataLen	-- 数据长度，以字节为单位，必须为AES_KEY_LENGTH/8的整倍数。
//				pIV			-- 初始化向量，如果使用ECB模式，可设为NULL。
//	输出参数：	pPlainText  -- 明文，即由密文解密后的数据，可以与pCipherText相同。
//	返回值：	无。
//////////////////////////////////////////////////////////////////////////
void AES_Decrypt(unsigned char *pPlainText, const unsigned char *pCipherText, 
				 unsigned int nDataLen, const unsigned char *pIV);


#ifdef __cplusplus
	}
#endif


#endif	// __AES_H
