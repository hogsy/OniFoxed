/*
	stack_walk_windows.c
*/

// 030800 : generates correct stack trace now -stefan
// 031700 : now looks for a valid map file named DEFAULT_MAP_FILE_NAME to do symbols lookups.
//          if no map file is found, you only get the hex addresses
// 083100 : Oni mods


#if defined(SHIPPING_VERSION) && (SHIPPING_VERSION != 1)

/*---------- headers */

#include "BFW.h"

#include "stack_walk_windows.h"

#include "imagehlp.h"
#include "tlhelp32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma optimize ("", off) // doesn't tend to work well w/ VC's "optimizer"

/*---------- constants */

enum
{
	MAXIMUM_SYMBOL_NAME_LENGTH= 256,
	MAX_MODULES= 128,
	MAX_FRAMES= 64,
	MAX_SYMBOL_NAME= 128,
	MAX_LINE_LENGTH= 256,
	MAX_NUMBER_OF_SYMBOLS= 32768
};

/*---------- structures */

typedef struct SYMBOL {
	unsigned long address;
	char name[MAX_SYMBOL_NAME];
	unsigned long rva_base;
	char lib_object[MAX_SYMBOL_NAME];
} Symbol, *SymbolPtr;


typedef struct StackWalkInput
{
	DWORD proc_id;
	HANDLE process;
	HANDLE thread;
	CONTEXT *context; // can be NULL; a valid context would be passed for handling exceptions
	short levels_to_ignore;
	boolean done;
} StackWalkInput;

typedef struct ModuleEntry
{
	char image_name[MAX_PATH];
	char module_name[MAX_PATH];
	DWORD base_address;
	DWORD size;
} ModuleEntry;

/*---------- globals */

static boolean stack_walk_initalized= FALSE;
static StackWalkInput stack_walk_input;

/*---------- prototypes */

static void stack_walk_thread_proc(void *args);
static boolean get_logical_address(void *addr, char *module, long length, long *section, long *offset);
static SymbolPtr parse_map_file(char *filename, long *num_symbols, char *timestamp_str);
int __cdecl symbol_sort_proc(const void *elem1, const void *elem2);
static char *symbol_name_from_address(unsigned long address, SymbolPtr symbol_list, long num_symbols);
static char *exception_code_to_string(int exception_code);

/*---------- code */

void stack_walk_initialize(
	void)
{
	memset(&stack_walk_input, 0, sizeof(stack_walk_input));
	UUrStartupMessage("stack crawl generation code enabled");
	stack_walk_initalized= TRUE;
	
	return;
}

void stack_walk(
	short levels_to_ignore)
{
	static StackWalkInput in;

	if (stack_walk_initalized)
	{
		++levels_to_ignore; // don't print this function

		in.proc_id= GetCurrentProcessId();
		in.process= OpenProcess(PROCESS_ALL_ACCESS, FALSE, in.proc_id);
		in.context= NULL;
		in.levels_to_ignore= levels_to_ignore;
		in.done= FALSE;

		if (DuplicateHandle(in.process, GetCurrentThread(),
			in.process, &in.thread, THREAD_ALL_ACCESS, FALSE, DUPLICATE_SAME_ACCESS))
		{
			if (in.thread)
			{
				HANDLE sw_thread;
				DWORD thread_id;
				
				sw_thread= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stack_walk_thread_proc,
					&in, 0, &thread_id);
				if (sw_thread)
				{
					SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );
					while (!in.done) // wait for stack walk to complete
					{
						Sleep(10);
					}
					SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
					CloseHandle(sw_thread);
				}
				else
				{
					UUrDebuggerMessage("could not create stack trace thread");
				}
			}
			else
			{
				UUrDebuggerMessage("no thread to perform a stack trace on");
			}
		}
		else
		{
			UUrDebuggerMessage("could not duplicate thread handle to perform stack trace");
		}
	}
	else
	{
		UUrDebuggerMessage("stack walk code not initialized");
	}

	return;
}

// same as stack_walk(), but takes a Windows CONTEXT to use for obtaining the stack info
// instead of grabbing it from the current thread
void stack_walk_with_context(
	short levels_to_ignore,
	CONTEXT *context)
{
	if (stack_walk_initalized)
	{
		// need 1 additional frame ++levels_to_ignore;
		stack_walk_input.proc_id= GetCurrentProcessId();
		stack_walk_input.process= OpenProcess(PROCESS_ALL_ACCESS, FALSE, stack_walk_input.proc_id);
		stack_walk_input.context= context;
		stack_walk_input.levels_to_ignore= levels_to_ignore;
		stack_walk_input.done= FALSE;

		if (DuplicateHandle(stack_walk_input.process, GetCurrentThread(),
			stack_walk_input.process, &stack_walk_input.thread,
			THREAD_ALL_ACCESS, FALSE, DUPLICATE_SAME_ACCESS))
		{
			if (stack_walk_input.thread)
			{
				HANDLE sw_thread;
				DWORD thread_id;
				
				sw_thread= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stack_walk_thread_proc,
					&stack_walk_input, 0, &thread_id);
				if (sw_thread)
				{
					SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );
					while (!stack_walk_input.done) // wait for stack walk to complete
					{
						Sleep(10);
					}
					SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
					CloseHandle(sw_thread);
				}
				else
				{
					UUrDebuggerMessage("could not create stack trace thread");
				}
			}
			else
			{
				UUrDebuggerMessage("no thread to perform a stack trace on");
			}
		}
		else
		{
			UUrDebuggerMessage("could not duplicate thread handle to perform stack trace");
		}
	}
	else
	{
		UUrDebuggerMessage("stack walk code not initialized");
	}

	return;
}

int handle_exception(
	LPEXCEPTION_POINTERS exception_data)
{	
	int ret= EXCEPTION_CONTINUE_SEARCH;

	if (exception_data)
	{
		if (exception_data->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
			ret= EXCEPTION_CONTINUE_EXECUTION;
		}
		else
		{
			UUrDebuggerMessage("exception %s (code= %lX) at 0x%lX",
				exception_code_to_string(exception_data->ExceptionRecord->ExceptionCode),
				exception_data->ExceptionRecord->ExceptionCode,
				exception_data->ExceptionRecord->ExceptionAddress);
			stack_walk_with_context(0, exception_data->ContextRecord);
		}
	}
	else
	{
		stack_walk(0);
	}

	return ret;
}

static void stack_walk_thread_proc(
	void *args)
{
	CONTEXT context= {0};

	/* this is happening in OniTool ... why?
	if (args != (void *)(&stack_walk_input))
	{
		UUrDebuggerMessage("memory has been corrupted");
	}*/
	if ((stack_walk_input.thread == 0) || (stack_walk_input.process == 0) ||
		(stack_walk_input.done != 0))
	{
		UUrDebuggerMessage("bad input to stack_walk_thread_proc()");
	}
	else
	{
		// put patient under before operating on it
		if (SuspendThread(stack_walk_input.thread) != -1L) // this might mean we're already running in a debugger
		{
			if (SymInitialize(stack_walk_input.process, NULL, FALSE))
			{
				DWORD opts= SymGetOptions();
				boolean success= FALSE;
				
				opts|= (SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS);
				SymSetOptions(opts);

			#ifdef DISPLAY_SYMBOLS_INFO_IN_STACK_TRACE
				// enumerate, load modules
				load_module_symbols(stack_walk_input.process, stack_walk_input.proc_id);
			#else // load blam's symbols only (map file will be needed)
				if (SymLoadModule(stack_walk_input.process, 0, __argv[0], NULL, 0, 0))
				{
					success= TRUE;
				}
				else
				{
					UUrDebuggerMessage("Error %lu loading symbols for \"%s\"",
						GetLastError(), __argv[0]);
				}
			#endif
				if (success && stack_walk_input.context)
				{
					context= *(stack_walk_input.context);
					success= TRUE;
				}
				else
				{
					context.ContextFlags= CONTEXT_CONTROL;
					success= GetThreadContext(stack_walk_input.thread, &context);
				}

				if (success)
				{
					char formatted_address[MAX_FRAMES][MAXIMUM_SYMBOL_NAME_LENGTH+128]; // text buffer
					STACKFRAME stack_frame;
					short frame_num;

					memset(formatted_address, 8, MAX_FRAMES*(MAXIMUM_SYMBOL_NAME_LENGTH+128));

					// Initialize the stack_frame structure for the first call.  This is only
					// necessary for Intel CPUs, and isn't mentioned in the documentation.
					UUrDebuggerMessage("starting stack trace");
			
					memset(&stack_frame, 0, sizeof(stack_frame));
					stack_frame.AddrPC.Offset= context.Eip;
					stack_frame.AddrPC.Mode= AddrModeFlat;
					stack_frame.AddrStack.Offset= context.Esp;
					stack_frame.AddrStack.Mode= AddrModeFlat;
					stack_frame.AddrFrame.Offset= context.Ebp;
					stack_frame.AddrFrame.Mode= AddrModeFlat;

					for (frame_num = 0; frame_num < MAX_FRAMES; ++frame_num)
					{					
						BYTE symbol_buffer[sizeof(IMAGEHLP_SYMBOL)+MAXIMUM_SYMBOL_NAME_LENGTH]= {0};
						PIMAGEHLP_SYMBOL p_symbol= (PIMAGEHLP_SYMBOL)symbol_buffer;
						IMAGEHLP_MODULE module;
						DWORD displacement= 0;

						memset(&module, 0, sizeof(module));

						// get next stack frame
						// if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
						// assume that either you are done, or that the stack is so hosed that the next
						// deeper frame could not be found.

						if (!StackWalk(IMAGE_FILE_MACHINE_I386, stack_walk_input.process,
							stack_walk_input.thread,
							&stack_frame, NULL, NULL, SymFunctionTableAccess,
							SymGetModuleBase, NULL))
						{
							UUrDebuggerMessage("couldn't get next stack frame");
							break;
						}
						// no return address means no deeper stackframe
						if (stack_frame.AddrReturn.Offset == 0)
						{
							SetLastError(0);
							break;
						}

						if (stack_frame.AddrFrame.Offset == 0) // sanity check to make sure that stack frame is ok
						{
							UUrDebuggerMessage("stack is corrupt ... ending stack trace : error= %lu", GetLastError());
							break;
						}

					#ifdef DISPLAY_SYMBOLS_INFO_IN_STACK_TRACE
						p_symbol->SizeOfStruct= sizeof(IMAGEHLP_SYMBOL);
						p_symbol->MaxNameLength= MAXIMUM_SYMBOL_NAME_LENGTH;
						if (SymGetSymFromAddr(in->process, stack_frame.AddrPC.Offset, &displacement, p_symbol))
						{
							char *sym_type= "";

							// show module info
							if (SymGetModuleInfo(in->process, stack_frame.AddrPC.Offset, &module))
							{
								switch ( module.SymType )
								{
									case SymNone: sym_type= "-nosymbols-"; break;
									case SymCoff: sym_type= "COFF"; break;
									case SymCv: sym_type= "CV"; break;
									case SymPdb: sym_type= "PDB"; break;
									case SymExport: sym_type= "-exported-"; break;
									case SymDeferred: sym_type= "-deferred-"; break;
									case SymSym: sym_type= "SYM"; break;
									default: sym_type= "<unknown>"; break;
								}
								sprintf(formatted_address[frame_num], "%08x %s+%04x Mod: %s[%s] Sym: type: %s, file: %s",
									stack_frame.AddrPC.Offset, p_symbol->Name, displacement,
									module.module_name, module.image_name, sym_type, module.Loadedimage_name);
							}
							else
							{
								sprintf(formatted_address[frame_num], "%08x %s+%04x [no module info available]",
									stack_frame.AddrPC.Offset, p_symbol->Name, displacement);
							}
						}
						else
						{
							CHAR module[MAX_PATH];
							DWORD section= 0, offset= 0, sgsfa_error;

							sgsfa_error= GetLastError();
					
							get_logical_address((PVOID)(stack_frame.AddrPC.Offset), module, MAX_PATH, &section, &offset);
							sprintf(formatted_address[frame_num], "%08x %s+%04x [SymGetSymFromAddr() returned %ld]",
								section, module, offset, sgsfa_error);
						}
					#else
						sprintf(formatted_address[frame_num], "%08lX", stack_frame.AddrPC.Offset);
					#endif
					}
					// print out stack trace in reverse order (top->bottom)
					{
						short i= frame_num-1, j=0;
						SymbolPtr symbol_list;
						long num_symbols= 0;
						unsigned long checksum= 'Oni!';
						char timestamp_string[128]= "";

						//sprintf(timestamp_string, __DATE__, " ", __TIME__);
						strcat(timestamp_string, __DATE__);
						strcat(timestamp_string, " ");
						strcat(timestamp_string, __TIME__);

						symbol_list= parse_map_file(ONI_MAP_FILE, &num_symbols, timestamp_string);

						while (i>=stack_walk_input.levels_to_ignore)
						{
							unsigned long addr= strtoul(formatted_address[i], NULL, 16);
							
							UUrDebuggerMessage("%d: %s %s", j, formatted_address[i],
								(symbol_list ? symbol_name_from_address(addr, symbol_list, num_symbols) : ""));
							checksum^= addr;
							--i;
							++j;
						}

						if (symbol_list) free(symbol_list);
						
						UUrDebuggerMessage("end of stack trace : stack trace checksum= 0x%lX", checksum);
					}
				}
			}
			else
			{
				UUrDebuggerMessage("SymInitialize() failed, or input process was NULL");
			}
		}
		else
		{
			UUrDebuggerMessage("unable to suspend thread - maybe you are already in a debugger?");
		}

		SymCleanup(stack_walk_input.process);
		// revive the patient
		stack_walk_input.done= TRUE;
		ResumeThread(stack_walk_input.thread);
	}

	return;
}

static boolean get_logical_address(
	void *addr, 
	char *module,
	long length,
	long *section,
	long *offset)
{
	MEMORY_BASIC_INFORMATION mbi;
	boolean success= FALSE;

	if (VirtualQuery(addr, &mbi, sizeof(mbi)))
	{
		DWORD h_mod= (DWORD)mbi.AllocationBase;

		if (GetModuleFileName((HMODULE)h_mod, module, length))
		{
			PIMAGE_DOS_HEADER dos_header= (PIMAGE_DOS_HEADER)h_mod;
			PIMAGE_NT_HEADERS nt_header= (PIMAGE_NT_HEADERS)(h_mod+dos_header->e_lfanew);
			PIMAGE_SECTION_HEADER section_header= IMAGE_FIRST_SECTION(nt_header);
			DWORD rva= (DWORD)addr-h_mod;

			unsigned int i;

			for (i= 0; i<nt_header->FileHeader.NumberOfSections; ++i, ++section_header)
			{
				DWORD section_start= section_header->VirtualAddress;
				DWORD section_end= section_start+max(section_header->SizeOfRawData, section_header->Misc.VirtualSize);

				if (rva>=section_start && rva<=section_end)
				{
					*section= i+1;
					*offset= rva-section_start;
					success= TRUE;
					break;
				}
			}
		}
	}

	return success;
}

static SymbolPtr parse_map_file(
	char *filename,
	long *num_symbols,
	char *timestamp_str)
{
	FILE *file_ptr= NULL;
	SymbolPtr symbol_list= NULL;
	long i;

	*num_symbols= 0;

	if ((file_ptr= fopen(filename, "r")) == NULL)
	{
		UUrDebuggerMessage("map file '%s' not found", filename);
		UUrDebuggerMessage("executable timestamp : %s", timestamp_str);
		return NULL;
	}
	else
	{
		char line[MAX_LINE_LENGTH]= "";
		
		// display filename
		if (fgets(line, MAX_LINE_LENGTH, file_ptr))
		{
			UUrDebuggerMessage("reading symbols info from '%s' for '%s'",
				filename, line);
		}
		else
		{
			goto DONE;
		}

		// symbol data comes after the blank line following the header line:
		//  Address         Publics by Value              Rva+Base     Lib:Object
		for (;;)
		{
			if (fgets(line, MAX_LINE_LENGTH, file_ptr))
			{
				if (strstr(line, "Lib:Object"))
					break;
				else if (strstr(line, "Timestamp"))
				{
					UUrDebuggerMessage("executable timestamp : %s", timestamp_str);
					UUrDebuggerMessage("map file timestamp : %s", line);
				}
			}
			else
			{
				UUrDebuggerMessage("map file appears corrupt");
				goto DONE;
			}
		}

		symbol_list = (SymbolPtr)calloc(MAX_NUMBER_OF_SYMBOLS, sizeof(Symbol));
		if (!symbol_list)
			goto DONE;
		
		// build the symbol list
		for (i=0;i<MAX_NUMBER_OF_SYMBOLS;i++)
		{
			if (fgets(line, MAX_LINE_LENGTH, file_ptr))
			{
				char *delimiters= " \t\n\r";
				char *tok, *end_str= NULL;
				
				// get address
				tok= strtok(line, ":");
				if (tok && *tok==' ') // a single whitespace ' ' should preceed each valid line
				{
					tok= strtok(NULL, delimiters);
					if (tok)
					{
						symbol_list[i].address= strtoul(tok, &end_str, 16);
					}
					else
					{
						UUrDebuggerMessage("map file appears corrupt");
						free(symbol_list);
						symbol_list= NULL;
						break;
					}

					// get fxn
					tok= strtok(NULL, delimiters);
					if (tok)
					{
						sprintf(symbol_list[i].name, tok);
					}
					else if (strstr(line, "entry point at"))
					{
						// reading the static symbols part now
						// 1 blank line...
						fgets(line, MAX_LINE_LENGTH, file_ptr);
						if (!isspace(line[0]))
						{
							UUrDebuggerMessage("map file appears corrupt");
							free(symbol_list);
							symbol_list= NULL;
							break;
						}
						// followed by ' Static symbols' ...
						fgets(line, MAX_LINE_LENGTH, file_ptr);
						if (!strstr(line, "Static symbols"))
						{
							UUrDebuggerMessage("map file appears corrupt");
							free(symbol_list);
							symbol_list= NULL;
							break;
						}
						// followed by another blank line ...
						fgets(line, MAX_LINE_LENGTH, file_ptr);
						if (!isspace(line[0]))
						{
							UUrDebuggerMessage("map file appears corrupt");
							free(symbol_list);
							symbol_list= NULL;
							break;
						}
						// followed by standard entries again
						fgets(line, MAX_LINE_LENGTH, file_ptr);
						tok= strtok(line, ":");
						if (tok && *tok==' ') // a single whitespace ' ' should preceed each valid line
						{
							tok= strtok(NULL, delimiters);
							if (tok)
							{
								symbol_list[i].address= strtoul(tok, &end_str, 16);
							}
							else
							{
								UUrDebuggerMessage("map file appears corrupt");
								free(symbol_list);
								symbol_list= NULL;
								break;
							}

							// get fxn
							tok= strtok(NULL, delimiters);
							if (tok)
							{
								sprintf(symbol_list[i].name, tok);
							}
						}
						else
						{
							UUrDebuggerMessage("map file appears corrupt");
							free(symbol_list);
							symbol_list= NULL;
							break;
						}
					}
					else
					{
						UUrDebuggerMessage("map file appears corrupt");
						free(symbol_list);
						symbol_list= NULL;
						break;
					}

					// get Rva + base
					tok= strtok(NULL, delimiters);
					if (tok)
					{
						symbol_list[i].rva_base= strtoul(tok, &end_str, 16);
					}
					else
					{
						UUrDebuggerMessage("map file appears corrupt");
						free(symbol_list);
						symbol_list= NULL;
						break;
					}

					// skip the f/i specifiers (5 characters)
					// " f i ", " f   "
					if (!end_str)
					{
						UUrDebuggerMessage("map file appears corrupt");
						free(symbol_list);
						symbol_list= NULL;
						break;
					}
					end_str+=5;

					// get the object file name
					tok= strtok(end_str, delimiters);
					if (tok)
					{
						sprintf(symbol_list[i].lib_object, tok);
					}
					else
					{
						UUrDebuggerMessage("map file appears corrupt");
						free(symbol_list);
						symbol_list= NULL;
						break;
					}

				} // else, maybe a blank line?		
			}
			else
			{
				// done reading map file
				break;
			}
		}
	}

DONE:

	fclose(file_ptr);
	
	if (symbol_list)
	{
		*num_symbols= i;
		if (i>=MAX_NUMBER_OF_SYMBOLS)
			UUrDebuggerMessage("there were more than %ld symbols to read; %ld were read\n", MAX_NUMBER_OF_SYMBOLS, i);
		// sort list, eliminate entries w/ address of '0'
		qsort(symbol_list, *num_symbols, sizeof(Symbol), symbol_sort_proc);
		while (symbol_list[*num_symbols-1].rva_base == 0)
			--(*num_symbols);
	}

	return symbol_list;
}

int __cdecl symbol_sort_proc(
	const void *elem1,
	const void *elem2)
{
	SymbolPtr s1= (SymbolPtr)(elem1), s2= (SymbolPtr)(elem2);
	// sort 0 addresses to the end of the list; otherwise sort ascending
	if ((s1->rva_base == 0) || (s1->rva_base > s2->rva_base))
		return 1;
	if ((s2->rva_base == 0) || (s1->rva_base < s2->rva_base))
		return -1;
	return 0;
}

static char *symbol_name_from_address(
	unsigned long address,
	SymbolPtr symbol_list,
	long num_symbols)
{
	static char symbol_name[MAX_SYMBOL_NAME];

	if (!symbol_list)
	{
		sprintf(symbol_name, "<additional information not available>");
	}
	else if (address >= symbol_list[0].rva_base && address < symbol_list[num_symbols-1].rva_base+0xFFFF)
	{
 		long i;

		for (i=1; i<num_symbols; i++)
		{
			if (symbol_list[i-1].rva_base <= address && address < symbol_list[i].rva_base)
			{
				sprintf(symbol_name, "%s + %04lX : %s", symbol_list[i-1].name,
					(address-symbol_list[i-1].rva_base), symbol_list[i-1].lib_object);
				break;
			}
		}
		if (i>= num_symbols)
			sprintf(symbol_name, "<no additional symbol information found>");
	}
	else
	{
		sprintf(symbol_name, "<invalid address; symbols should be between %08lX and %08lX>",
			symbol_list[0].rva_base, symbol_list[num_symbols-1].rva_base);
	}

	return symbol_name;
}

#define CODE_TO_STR(code) case code: str= #code; break;
static char *exception_code_to_string(
	int exception_code)
{
	static char *str= "unknown exception";

	switch (exception_code)
	{
		CODE_TO_STR(EXCEPTION_ACCESS_VIOLATION)
		CODE_TO_STR(EXCEPTION_DATATYPE_MISALIGNMENT)
		CODE_TO_STR(EXCEPTION_BREAKPOINT)
		CODE_TO_STR(EXCEPTION_SINGLE_STEP)
		CODE_TO_STR(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
		CODE_TO_STR(EXCEPTION_FLT_DENORMAL_OPERAND)
		CODE_TO_STR(EXCEPTION_FLT_DIVIDE_BY_ZERO)
		CODE_TO_STR(EXCEPTION_FLT_INEXACT_RESULT)
		CODE_TO_STR(EXCEPTION_FLT_INVALID_OPERATION)
		CODE_TO_STR(EXCEPTION_FLT_OVERFLOW)
		CODE_TO_STR(EXCEPTION_FLT_STACK_CHECK)
		CODE_TO_STR(EXCEPTION_FLT_UNDERFLOW)
		CODE_TO_STR(EXCEPTION_INT_DIVIDE_BY_ZERO)
		CODE_TO_STR(EXCEPTION_INT_OVERFLOW)
		CODE_TO_STR(EXCEPTION_PRIV_INSTRUCTION)
		CODE_TO_STR(EXCEPTION_IN_PAGE_ERROR)
		CODE_TO_STR(EXCEPTION_ILLEGAL_INSTRUCTION)
		CODE_TO_STR(EXCEPTION_NONCONTINUABLE_EXCEPTION)
		CODE_TO_STR(EXCEPTION_STACK_OVERFLOW)
		CODE_TO_STR(EXCEPTION_INVALID_DISPOSITION)
		CODE_TO_STR(EXCEPTION_GUARD_PAGE)
		CODE_TO_STR(EXCEPTION_INVALID_HANDLE)
	}

	return str;
}

#pragma optimize ("", on)

#endif // SHIPPING_VERSION

