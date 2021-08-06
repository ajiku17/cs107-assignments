#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"
#include "branch.h"

/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  // printf("operating in branch %d \n", (int)AccountNum_GetBranchID(accountNum));
  // printf("deposit on account; %lu\n", account->accountNumber);

  sem_wait(account->accountDataIsBeingProcessed);
  // // sem_wait(bank->bankDataIsBeingProcessed);
  int branchId = AccountNum_GetBranchID(accountNum);
  Branch* active = &(bank->branches[branchId]);
  sem_wait(active->branchDataIsBeingProcessed);

  Account_Adjust(bank,account, amount, 1);
  
  sem_post(active->branchDataIsBeingProcessed);
  // sem_post(bank->bankDataIsBeingProcessed);
  sem_post(account->accountDataIsBeingProcessed);
  
  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  
  sem_wait(account->accountDataIsBeingProcessed);
  // sem_wait(bank->bankDataIsBeingProcessed);
  int branchId = AccountNum_GetBranchID(accountNum);
  Branch* active = &(bank->branches[branchId]);
  sem_wait(active->branchDataIsBeingProcessed);

  if (amount > Account_Balance(account)) {
    sem_post(account->accountDataIsBeingProcessed);
    // sem_post(bank->bankDataIsBeingProcessed);
    sem_post(active->branchDataIsBeingProcessed);
    return ERROR_INSUFFICIENT_FUNDS;
  }


  // printf("operating in branch %d \n", (int)AccountNum_GetBranchID(accountNum));
  // printf("withdraw on account; %lu\n", account->accountNumber);
  Account_Adjust(bank,account, -amount, 1);

  sem_post(account->accountDataIsBeingProcessed);
  // sem_post(bank->bankDataIsBeingProcessed);
  sem_post(active->branchDataIsBeingProcessed);

  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if(srcAccount->accountNumber == dstAccount->accountNumber){
    return ERROR_SUCCESS;
  }
  
  sem_wait(bank->bankDataIsBeingProcessed);

  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);  

  if(Account_IsSameBranch(srcAccountNum, dstAccountNum)){
    if(srcAccount->accountNumber < dstAccount->accountNumber){
      sem_wait(srcAccount->accountDataIsBeingProcessed);
      sem_wait(dstAccount->accountDataIsBeingProcessed);
    }else{
      sem_wait(dstAccount->accountDataIsBeingProcessed);
      sem_wait(srcAccount->accountDataIsBeingProcessed);
    }
  }else{
    int srcBranchId = AccountNum_GetBranchID(srcAccountNum);
    int dstBranchId = AccountNum_GetBranchID(dstAccountNum);
    // printf("cross branch tranfer start: %d -> %d\n",srcBranchId, dstBranchId);
    if(srcBranchId < dstBranchId){
      sem_wait(srcAccount->accountDataIsBeingProcessed);
      sem_wait(dstAccount->accountDataIsBeingProcessed);
      sem_wait(bank->branches[srcBranchId].branchDataIsBeingProcessed);
      sem_wait(bank->branches[dstBranchId].branchDataIsBeingProcessed);
    }else{
      sem_wait(dstAccount->accountDataIsBeingProcessed);
      sem_wait(srcAccount->accountDataIsBeingProcessed);
      sem_wait(bank->branches[dstBranchId].branchDataIsBeingProcessed);
      sem_wait(bank->branches[srcBranchId].branchDataIsBeingProcessed);
    }
  }
  

  // sem_wait(bank->bankDataIsBeingProcessed);
  // int branchId = AccountNum_GetBranchID(srcAccountNum);
  // Branch* active = &(bank->branches[branchId]);
  // sem_wait(active->branchDataIsBeingProcessed);
  
  if (amount > Account_Balance(srcAccount)) {
    sem_post(srcAccount->accountDataIsBeingProcessed);
    sem_post(dstAccount->accountDataIsBeingProcessed);
    if(!Account_IsSameBranch(srcAccountNum, dstAccountNum)){
      int srcBranchId = AccountNum_GetBranchID(srcAccountNum);
      int dstBranchId = AccountNum_GetBranchID(dstAccountNum);
      sem_post(bank->branches[dstBranchId].branchDataIsBeingProcessed);
      sem_post(bank->branches[srcBranchId].branchDataIsBeingProcessed);
    }
    sem_post(bank->bankDataIsBeingProcessed);
    // printf("insufficient funds: post on %lu\n", srcAccount->accountNumber);
    return ERROR_INSUFFICIENT_FUNDS;
  }

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  

  Account_Adjust(bank, srcAccount, -amount, updateBranch);
  Account_Adjust(bank, dstAccount, amount, updateBranch);

  sem_post(srcAccount->accountDataIsBeingProcessed);
  sem_post(dstAccount->accountDataIsBeingProcessed);
  if(!Account_IsSameBranch(srcAccountNum, dstAccountNum)){
    int srcBranchId = AccountNum_GetBranchID(srcAccountNum);
    int dstBranchId = AccountNum_GetBranchID(dstAccountNum);
    sem_post(bank->branches[dstBranchId].branchDataIsBeingProcessed);
    sem_post(bank->branches[srcBranchId].branchDataIsBeingProcessed);
  }

  sem_post(bank->bankDataIsBeingProcessed);
  
  return ERROR_SUCCESS;
}
