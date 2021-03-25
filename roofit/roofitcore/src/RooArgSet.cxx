/***************************************************************************** * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
/// \class RooArgSet
/// RooArgSet is a container object that can hold multiple RooAbsArg objects.
/// The container has set semantics which means that:
///
///  - Every object it contains must have a unique name returned by GetName().
///
///  - Contained objects are not ordered, although the set can be traversed
///    using an iterator returned by createIterator(). The iterator does not
///    necessarily follow the object insertion order.
///
///  - Objects can be retrieved by name only, and not by index.
///
///
/// Ownership of contents
/// -------------------------
/// Unowned objects are inserted with the add() method. Owned objects
/// are added with addOwned() or addClone(). A RooArgSet either owns all
/// of it contents, or none, which is determined by the first `add`
/// call. Once an ownership status is selected, inappropriate `add` calls
/// will return error status. Clearing the list via removeAll() resets the
/// ownership status. Arguments supplied in the constructor are always added
/// as unowned elements.
///
///

#include "RooArgSet.h"

#include "Riostream.h"
#include "TClass.h"
#include "RooErrorHandler.h"
#include "RooStreamParser.h"
#include "RooFormula.h"
#include "RooAbsRealLValue.h"
#include "RooAbsCategoryLValue.h"
#include "RooStringVar.h"
#include "RooTrace.h"
#include "RooArgList.h"
#include "RooSentinel.h"
#include "RooMsgService.h"
#include "ROOT/RMakeUnique.hxx"

#include <iomanip>

using namespace std ;

#if (__GNUC__==3&&__GNUC_MINOR__==2&&__GNUC_PATCHLEVEL__==3)
char* operator+( streampos&, char* );
#endif

ClassImp(RooArgSet);



#ifndef USEMEMPOOLFORARGSET
void RooArgSet::cleanup() { }
#else

#include "MemPoolForRooSets.h"

RooArgSet::MemPool* RooArgSet::memPool() {
  RooSentinel::activate();
  static auto * memPool = new RooArgSet::MemPool();
  return memPool;
}

////////////////////////////////////////////////////////////////////////////////
/// Clear memory pool on exit to avoid reported memory leaks

void RooArgSet::cleanup()
{
  auto pool = memPool();
  memPool()->teardown();

  //Here, the pool might have to leak if RooArgSets are still alive.
  if (pool->empty())
    delete pool;
}


////////////////////////////////////////////////////////////////////////////////
/// Overloaded new operator guarantees that all RooArgSets allocated with new
/// have a unique address, a property that is exploited in several places
/// in roofit to quickly index contents on normalization set pointers. 
/// The memory pool only allocates space for the class itself. The elements
/// stored in the set are stored outside the pool.

void* RooArgSet::operator new (size_t bytes)
{
  //This will fail if a derived class uses this operator
  assert(sizeof(RooArgSet) == bytes);

  return memPool()->allocate(bytes);
}


////////////////////////////////////////////////////////////////////////////////
/// Overloaded new operator with placement does not guarante that all
/// RooArgSets allocated with new have a unique address, but uses the global
/// operator.

void* RooArgSet::operator new (size_t bytes, void* ptr) noexcept
{
   return ::operator new (bytes, ptr);
}


////////////////////////////////////////////////////////////////////////////////
/// Memory is owned by pool, we need to do nothing to release it

void RooArgSet::operator delete (void* ptr)
{
  // Decrease use count in pool that ptr is on
  if (memPool()->deallocate(ptr))
    return;

  std::cerr << __func__ << " " << ptr << " is not in any of the pools." << std::endl;

  // Not part of any pool; use global op delete:
  ::operator delete(ptr);
}

#endif


////////////////////////////////////////////////////////////////////////////////
/// Default constructor

RooArgSet::RooArgSet() :
  RooAbsCollection()
{
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor from a RooArgList. If the list contains multiple
/// objects with the same name, only the first is store in the set.
/// Warning messages will be printed for dropped items.

RooArgSet::RooArgSet(const RooArgList& list) :
  RooAbsCollection(list.GetName())
{
  add(list,kTRUE) ; // verbose to catch duplicate errors
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor from a RooArgList. If the list contains multiple
/// objects with the same name, only the first is store in the set.
/// Warning messages will be printed for dropped items.

RooArgSet::RooArgSet(const RooArgList& list, const RooAbsArg* var1) :
  RooAbsCollection(list.GetName())
{
  if (var1 && !list.contains(*var1)) {
    add(*var1,kTRUE) ;
  }
  add(list,kTRUE) ; // verbose to catch duplicate errors
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Empty set constructor

RooArgSet::RooArgSet(const char *name) :
  RooAbsCollection(name)
{
  TRACE_CREATE
}




////////////////////////////////////////////////////////////////////////////////
/// Construct a set from two existing sets

RooArgSet::RooArgSet(const RooArgSet& set1, const RooArgSet& set2, const char *name) : RooAbsCollection(name)
{
  add(set1) ;
  add(set2) ;
  TRACE_CREATE    
}




////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 1 initial object

RooArgSet::RooArgSet(const RooAbsArg& var1,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 2 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 3 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2, 
		     const RooAbsArg& var3,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 4 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2, 
		     const RooAbsArg& var3, const RooAbsArg& var4,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3); add(var4);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 5 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1,
		     const RooAbsArg& var2, const RooAbsArg& var3,
		     const RooAbsArg& var4, const RooAbsArg& var5,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3); add(var4); add(var5);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 6 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2, 
		     const RooAbsArg& var3, const RooAbsArg& var4, 
		     const RooAbsArg& var5, const RooAbsArg& var6,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3); add(var4); add(var5); add(var6);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 7 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2, 
		     const RooAbsArg& var3, const RooAbsArg& var4, 
		     const RooAbsArg& var5, const RooAbsArg& var6, 
		     const RooAbsArg& var7,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3); add(var4); add(var5); add(var6); add(var7) ;
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 8 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2, 
		     const RooAbsArg& var3, const RooAbsArg& var4, 
		     const RooAbsArg& var5, const RooAbsArg& var6, 
		     const RooAbsArg& var7, const RooAbsArg& var8,
		     const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3); add(var4); add(var5); add(var6); add(var7) ;add(var8) ;
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor for set containing 9 initial objects

RooArgSet::RooArgSet(const RooAbsArg& var1, const RooAbsArg& var2, 
		     const RooAbsArg& var3, const RooAbsArg& var4, 
		     const RooAbsArg& var5, const RooAbsArg& var6, 
		     const RooAbsArg& var7, const RooAbsArg& var8,
		     const RooAbsArg& var9, const char *name) :
  RooAbsCollection(name)
{
  add(var1); add(var2); add(var3); add(var4); add(var5); add(var6); add(var7); add(var8); add(var9);
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Constructor from a root TCollection. Elements in the collection that
/// do not inherit from RooAbsArg will be skipped. A warning message
/// will be printed for every skipped item.

RooArgSet::RooArgSet(const TCollection& tcoll, const char* name) :
  RooAbsCollection(name)
{
  TIterator* iter = tcoll.MakeIterator() ;
  TObject* obj ;
  while((obj=iter->Next())) {
    if (!dynamic_cast<RooAbsArg*>(obj)) {
      coutW(InputArguments) << "RooArgSet::RooArgSet(TCollection) element " << obj->GetName() 
			    << " is not a RooAbsArg, ignored" << endl ;
      continue ;
    }
    add(*(RooAbsArg*)obj) ;
  }
  delete iter ;
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Copy constructor. Note that a copy of a set is always non-owning,
/// even the source set is owning. To create an owning copy of
/// a set (owning or not), use the snaphot() method.

RooArgSet::RooArgSet(const RooArgSet& other, const char *name) 
  : RooAbsCollection(other,name)
{
  TRACE_CREATE
}



////////////////////////////////////////////////////////////////////////////////
/// Destructor

RooArgSet::~RooArgSet() 
{
  TRACE_DESTROY  
}



////////////////////////////////////////////////////////////////////////////////
/// Add element to non-owning set. The operation will fail if
/// a similarly named object already exists in the set, or
/// the set is specified to own its elements. Eventual error messages
/// can be suppressed with the silent flag

Bool_t RooArgSet::add(const RooAbsArg& var, Bool_t silent) 
{
  return checkForDup(var,silent)? kFALSE : RooAbsCollection::add(var,silent) ;
}



////////////////////////////////////////////////////////////////////////////////
/// Add element to an owning set. The operation will fail if
/// a similarly named object already exists in the set, or
/// the set is not specified to own its elements. Eventual error messages
/// can be suppressed with the silent flag

Bool_t RooArgSet::addOwned(RooAbsArg& var, Bool_t silent)
{
  return checkForDup(var,silent)? kFALSE : RooAbsCollection::addOwned(var,silent) ;
}



////////////////////////////////////////////////////////////////////////////////
/// Add clone of specified element to an owning set. If sucessful, the
/// set will own the clone, not the original. The operation will fail if
/// a similarly named object already exists in the set, or
/// the set is not specified to own its elements. Eventual error messages
/// can be suppressed with the silent flag

RooAbsArg* RooArgSet::addClone(const RooAbsArg& var, Bool_t silent) 
{
  return checkForDup(var,silent)? 0 : RooAbsCollection::addClone(var,silent) ;
}



////////////////////////////////////////////////////////////////////////////////
/// Array operator. Named element must exist in set, otherwise
/// code will abort. 
///
/// When used as lvalue in assignment operations, the element contained in
/// the list will not be changed, only the value of the existing element!

RooAbsArg& RooArgSet::operator[](const char* name) const 
{     
  RooAbsArg* arg = find(name) ;
  if (!arg) {
    coutE(InputArguments) << "RooArgSet::operator[](" << GetName() << ") ERROR: no element named " << name << " in set" << endl ;
    RooErrorHandler::softAbort() ;
  }
  return *arg ; 
}



////////////////////////////////////////////////////////////////////////////////
/// Check if element with var's name is already in set

Bool_t RooArgSet::checkForDup(const RooAbsArg& var, Bool_t silent) const 
{
  RooAbsArg *other = find(var);
  if (other) {
    if (other != &var) {
      if (!silent) {
	// print a warning if this variable is not the same one we
	// already have
	coutE(InputArguments) << "RooArgSet::checkForDup: ERROR argument with name " << var.GetName() << " is already in this set" << endl;
      }
    }
    // don't add duplicates
    return kTRUE;
  }
  return kFALSE ;
}







////////////////////////////////////////////////////////////////////////////////
/// Write contents of the argset to specified file.
/// See writeToStream() for details

void RooArgSet::writeToFile(const char* fileName) const
{
  ofstream ofs(fileName) ;
  if (ofs.fail()) {
    coutE(InputArguments) << "RooArgSet::writeToFile(" << GetName() << ") error opening file " << fileName << endl ;
    return ;
  }
  writeToStream(ofs,kFALSE) ;
}



////////////////////////////////////////////////////////////////////////////////
/// Read contents of the argset from specified file.
/// See readFromStream() for details

Bool_t RooArgSet::readFromFile(const char* fileName, const char* flagReadAtt, const char* section, Bool_t verbose) 
{
  ifstream ifs(fileName) ;
  if (ifs.fail()) {
    coutE(InputArguments) << "RooArgSet::readFromFile(" << GetName() << ") error opening file " << fileName << endl ;
    return kTRUE ;
  }
  return readFromStream(ifs,kFALSE,flagReadAtt,section,verbose) ;
}




////////////////////////////////////////////////////////////////////////////////
/// Write the contents of the argset in ASCII form to given stream.
/// 
/// A line is written for each element contained in the form
/// `<argName> = <argValue>`
/// 
/// The `<argValue>` part of each element is written by the arguments'
/// writeToStream() function.
/// \param os The stream to write to.
/// \param compact Write only the bare values, separated by ' '.
/// \note In compact mode, the stream cannot be read back into a RooArgSet,
/// but only into a RooArgList, because the variable names are lost.
/// \param section If non-null, add a section header like `[<section>]`.
void RooArgSet::writeToStream(ostream& os, Bool_t compact, const char* section) const
{
  if (section && section[0] != '\0')
    os << '[' << section << ']' << '\n';

  if (compact) {
    for (const auto next : _list) {
      next->writeToStream(os, true);
      os << " ";
    }
    os << endl;
  } else {
    for (const auto next : _list) {
      os << next->GetName() << " = " ;
      next->writeToStream(os,kFALSE) ;
      os << endl ;
    }
  }
}




////////////////////////////////////////////////////////////////////////////////
/// Read the contents of the argset in ASCII form from given stream.
/// 
/// The stream is read to end-of-file and each line is assumed to be
/// of the form
/// \code
///   <argName> = <argValue>
/// \endcode
/// Lines starting with argNames not matching any element in the list
/// will be ignored with a warning message. In addition limited C++ style 
/// preprocessing and flow control is provided. The following constructions 
/// are recognized:
/// \code
///   include "include.file"
/// \endcode
/// Include given file, recursive inclusion OK
/// \code
/// if (<boolean_expression>)
///   <name> = <value>
///   ....
/// else if (<boolean_expression>)
///   ....
/// else
///   ....
/// endif
/// \endcode
///
/// All expressions are evaluated by RooFormula, and may involve any of
/// the sets variables. 
/// \code
///   echo <Message>
/// \endcode
/// Print console message while reading from stream
/// \code
///   abort
/// \endcode
/// Force termination of read sequence with error status 
///
/// The value of each argument is read by the arguments readFromStream
/// function.

Bool_t RooArgSet::readFromStream(istream& is, Bool_t compact, const char* flagReadAtt, const char* section, Bool_t verbose) 
{
  if (compact) {
    coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << ") compact mode not supported" << endl ;
    return kTRUE ;
  }

  RooStreamParser parser(is) ;
  parser.setPunctuation("=") ;
  TString token ;
  Bool_t retVal(kFALSE) ;
  
  // Conditional stack and related state variables
  // coverity[UNINIT]
  Bool_t anyCondTrue[100] ;
  Bool_t condStack[100] ;
  Bool_t lastLineWasElse=kFALSE ;
  Int_t condStackLevel=0 ;
  condStack[0]=kTRUE ;
  
  // Prepare section processing
  TString sectionHdr("[") ;
  if (section) sectionHdr.Append(section) ;
  sectionHdr.Append("]") ;
  Bool_t inSection(section?kFALSE:kTRUE) ;

  Bool_t reprocessToken = kFALSE ;
  while (1) {

    if (is.eof() || is.fail() || parser.atEOF()) {
      break ;
    }
    
    // Read next token until memEnd of file
    if (!reprocessToken) {
      token = parser.readToken() ;
    }
    reprocessToken = kFALSE ;

    // Skip empty lines 
    if (token.IsNull()) {
      continue ;
    }

    // Process include directives
    if (!token.CompareTo("include")) {
      if (parser.atEOL()) {
	coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() 
			      << "): no filename found after include statement" << endl ;
	return kTRUE ;
      }
      TString filename = parser.readLine() ;
      ifstream incfs(filename) ;
      if (!incfs.good()) {
	coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): cannot open include file " << filename << endl ;
	return kTRUE ;
      }
      coutI(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): processing include file " 
			    << filename << endl ;
      if (readFromStream(incfs,compact,flagReadAtt,inSection?0:section,verbose)) return kTRUE ;
      continue ;
    }

    // Process section headers if requested
    if (*token.Data()=='[') {
      TString hdr(token) ;
      const char* last = token.Data() + token.Length() -1 ;
      if (*last != ']') {
	hdr.Append(" ") ;
	hdr.Append(parser.readLine()) ;
      }
//       parser.putBackToken(token) ;
//       token = parser.readLine() ;
      if (section) {
	inSection = !sectionHdr.CompareTo(hdr) ;
      }
      continue ;
    }

    // If section is specified, ignore all data outside specified section
    if (!inSection) {
      parser.zapToEnd(kTRUE) ;
      continue ;
    }

    // Conditional statement evaluation
    if (!token.CompareTo("if")) {
      
      // Extract conditional expressions and check validity
      TString expr = parser.readLine() ;
      RooFormula form(expr,expr,*this) ;
      if (!form.ok()) return kTRUE ;
      
      // Evaluate expression
      Bool_t status = form.eval()?kTRUE:kFALSE ;
      if (lastLineWasElse) {
	anyCondTrue[condStackLevel] |= status ;
	lastLineWasElse=kFALSE ;
      } else {
	condStackLevel++ ;
	anyCondTrue[condStackLevel] = status ;
      }
      condStack[condStackLevel] = status ;
      
      if (verbose) cxcoutD(Eval) << "RooArgSet::readFromStream(" << GetName() 
				 << "): conditional expression " << expr << " = " 
				 << (condStack[condStackLevel]?"true":"false") << endl ;
      continue ; // go to next line
    }
    
    if (!token.CompareTo("else")) {
      // Must have seen an if statement before
      if (condStackLevel==0) {
	coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): unmatched 'else'" << endl ;
      }
      
      if (parser.atEOL()) {
	// simple else: process if nothing else was true
	condStack[condStackLevel] = !anyCondTrue[condStackLevel] ; 
	parser.zapToEnd(kFALSE) ;
	continue ;
      } else {
	// if anything follows it should be 'if'
	token = parser.readToken() ;
	if (token.CompareTo("if")) {
	  coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): syntax error: 'else " << token << "'" << endl ;
	  return kTRUE ;
	} else {
	  if (anyCondTrue[condStackLevel]) {
	    // No need for further checking, true conditional already processed
	    condStack[condStackLevel] = kFALSE ;
	    parser.zapToEnd(kFALSE) ;
	    continue ;
	  } else {
	    // Process as normal 'if' no true conditional was encountered 
	    reprocessToken = kTRUE ;
	    lastLineWasElse=kTRUE ;
	    continue ;
	  }
	}
      }	
    }
    
    if (!token.CompareTo("endif")) {
      // Must have seen an if statement before
      if (condStackLevel==0) {
	coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): unmatched 'endif'" << endl ;
	return kTRUE ;
      }
      
      // Decrease stack by one
      condStackLevel-- ;
      continue ;
    } 
    
    // If current conditional is true
    if (condStack[condStackLevel]) {
      
      // Process echo statements
      if (!token.CompareTo("echo")) {
	TString message = parser.readLine() ;
	coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): >> " << message << endl ;
	continue ;
      } 
      
      // Process abort statements
      if (!token.CompareTo("abort")) {
	TString message = parser.readLine() ;
	coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): USER ABORT" << endl ;
	return kTRUE ;
      } 
      
      // Interpret the rest as <arg> = <value_expr> 
      RooAbsArg *arg ;

      if ((arg = find(token)) && !arg->getAttribute("Dynamic")) {
	if (parser.expectToken("=",kTRUE)) {
	  parser.zapToEnd(kTRUE) ;
	  retVal=kTRUE ;
	  coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() 
				<< "): missing '=' sign: " << arg << endl ;
	  continue ;
	}
	Bool_t argRet = arg->readFromStream(is,kFALSE,verbose) ;	
	if (!argRet && flagReadAtt) arg->setAttribute(flagReadAtt,kTRUE) ;
	retVal |= argRet ;
      } else {
	if (verbose) {
	  coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): argument " 
				<< token << " not in list, ignored" << endl ;
	}
	parser.zapToEnd(kTRUE) ;
      }
    } else {
      parser.readLine() ;
    }
  }
  
  // Did we fully unwind the conditional stack?
  if (condStackLevel!=0) {
    coutE(InputArguments) << "RooArgSet::readFromStream(" << GetName() << "): missing 'endif'" << endl ;
    return kTRUE ;
  }
  
  return retVal ;
}


Bool_t RooArgSet::isInRange(const char* rangeSpec) 
{
  char buf[1024] ;
  strlcpy(buf,rangeSpec,1024) ;
  char* token = strtok(buf,",") ;
  
  TIterator* iter = createIterator() ;

  while(token) {

    Bool_t accept=kTRUE ;
    iter->Reset() ;
    RooAbsArg* arg ;
    while((arg=(RooAbsArg*)iter->Next())) {
      RooAbsRealLValue* lvarg = dynamic_cast<RooAbsRealLValue*>(arg) ;
      if (lvarg) {
	if (!lvarg->inRange(token)) {
	  accept=kFALSE ;
	  break ;
	}
      }
      // WVE MUST HANDLE RooAbsCategoryLValue ranges as well
    }
    if (accept) {
      delete iter ;
      return kTRUE ;
    }

    token = strtok(0,",") ;
  }

  delete iter ;
  return kFALSE ;
}
