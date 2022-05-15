#if _MSC_VER

#else
int
strcpy_s(char* destination, size_t destinationCount, const char* source) {
	strcpy(destination, source);
	return 0;
}

int
strncpy_s(char* destination, size_t destsz, const char* source, size_t count) {
	strncpy(destination, source, count);
	return 0;
}

int 
strcat_s(char* destination, size_t destsz, const char* source) {
   strcat(destination, source);
   return 0;
}

#define sprintf_s snprintf

#endif
