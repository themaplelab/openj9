/*******************************************************************************
 * Copyright (c) 2000, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/
#include "env/VerboseLog.hpp"
#include "optimizer/CogniWorklistOpt.hpp"
#include "il/SymbolReference.hpp"
#include "il/Node_inlines.hpp"

bool
TR_CogniWorklistOpt::shouldPerform() {
   return true;
}

/*
 * looks for function calls in the compiling method that are uses of crypto APIs
 * 
 *
 */
int32_t
TR_CogniWorklistOpt::perform() {

  if(comp()->getPersistentInfo()->getCogniCryptMode() == COGNICRYPT_MODE){
  
  char *classToSearch = feGetEnv("TR_ClassToSearch");
  
  //walking tree tops will be sufficient
  TR::TreeTop *tt = comp()->getStartTree();
  for(; tt; tt = tt->getNextTreeTop()){
    
    TR::Node *node = tt->getNode();
    
    if(node->getNumChildren() > 0){
      if(node->getOpCodeValue() == TR::treetop) // jump over TreeTop
	node = node->getFirstChild();
      
      if(node->getNumChildren() > 0 &&
	 (node->getFirstChild()->getOpCode().isFunctionCall() || node->getFirstChild()->getOpCode().isCall())){
	
	TR::Node *classNode = node->getFirstChild();
	TR::Symbol *symbol = classNode->getSymbolReference()->getSymbol();
	
	if(symbol->isResolvedMethod()){
	  TR::ResolvedMethodSymbol *method = symbol->castToResolvedMethodSymbol();
	  if(method){

	    TR_ResolvedMethod *m = method->getResolvedMethod();
	    char *sig = m->signatureChars();	    

	    //TODO: replace hardcoded search for method signatures containing JCA API packages
	    if(((strstr(sig, "java/security") != NULL)) || ((strstr(sig, "javax/crypto") != NULL))){
	      //compilee method and class
	      TR_ResolvedMethod *feMethod = comp()->getCurrentMethod();
	      char *compileeClass = feMethod->classNameChars();
	      printf("JITCLIENT: Found a method that uses a crypto API:  %s.%s(...)\n", compileeClass, feMethod->nameChars());
	      //one item is enough
	      if(strncmp(compileeClass, classToSearch, strlen(classToSearch)) == 0){
			//grab exact string                                                                              
			char compilee[strlen(classToSearch)];
			strncpy(compilee, compileeClass, strlen(classToSearch));

			printf("JITCLIENT: sending class to analyse: %s\n", compilee);
			try{
			  Client *client = comp()->getPersistentInfo()->getCogniCryptClient();
			  //TODO utilize protobuf features once protobufs are integrated into CogniCrypt
			  client->writeClient(TCP::ClientMsgType::clientRequestInitAnalysis, "INITANALYSIS\n");
			  
			  client->writeClient(TCP::ClientMsgType::clientRequestInitAnalysis, compilee);
			  client->writeClient(TCP::ClientMsgType::clientRequestInitAnalysis, "\n");
			  client->writeClient(TCP::ClientMsgType::clientRequestInitAnalysis, "END\n");
			  printf("JITCLIENT: done search...\n");
			  //TODO reenable, or find more reasonable way to turn it off after one success 
			  comp()->getPersistentInfo()->setCogniCryptMode(NON_COGNICRYPT_MODE);
			  
		}catch(...){
		  printf("Analysis request aborted");
		}
	      }
	      break;
	    }
	  }
	  
	}
	  }
	}
  }
  }
  return 1;
}
