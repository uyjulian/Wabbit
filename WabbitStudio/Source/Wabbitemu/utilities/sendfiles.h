#ifndef SENDFILES_H
#define SENDFILES_H

int SizeofFileList(char* );
char* AppendName(char* , char* );
__declspec(dllexport) void SendFile( char* , int );
void SendFiles( char* , int );
void ThreadSend( char* , int);
void NoThreadSend( const char*, int);

#endif
