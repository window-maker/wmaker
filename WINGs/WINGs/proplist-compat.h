/*
 * This header file is provided for old libPropList compatibility.
 * DO _NOT_ USE this except for letting your old libPropList-based code to
 * work with the new property list code from WINGs, with minimal changes.
 *
 * All code written with old libPropList functions should work, given
 * that the following changes are made:
 *
 * 1. Replace all
 *       #include <proplist.h>
 *    with
 *       #include <WINGs/proplist-compat.h>
 *    in your code.
 *
 * 2. Change all calls to PLSave() to have the extra filename parameter like:
 *       PLSave(proplist_t proplist, char* filename, Bool atomically)
 *
 * 3. The PLSetStringCmpHook() function is no longer available. There is a
 *    similar but simpler function provided which is enough for practical
 *    purposes:
 *       PLSetCaseSensitive(Bool caseSensitive)
 *
 * 4. The following functions do no longer exist. They were removed because
 *    they were based on concepts borrowed from UserDefaults which conflict
 *    with the retain/release mechanism:
 *       PLSynchronize(), PLDeepSynchronize(), PLShallowSynchronize()
 *       PLSetFilename(), PLGetFilename()
 *       PLGetContainer()
 *    You should change your code to not use them anymore.
 *
 * 5. The following functions are no longer available. They were removed
 *    because they also used borrowed concepts which have no place in a
 *    property list as defined in the OpenStep specifications. Also these
 *    functions were hardly ever used in programs to our knowledge.
 *       PLGetDomainNames(), PLGetDomain(), PLSetDomain(), PLDeleteDomain()
 *       PLRegister(), PLUnregister()
 *    You should also change your code to not use them anymore (in case you
 *    ever used them anyway ;-) ).
 *
 * 6. Link your program with libWINGs or libWUtil instead of libPropList.
 *    (libWINGs should be used for GUI apps, while libWUtil for non-GUI apps)
 *
 *
 * Our recommandation is to rewrite your code to use the new functions and
 * link against libWINGs/libWUtil. We do not recommend you to keep using old
 * libPropList function names. This file is provided just to allow existing
 * libropList based applications to run with minimal changes with the new
 * proplist code from WINGs before their authors get the time to rewrite
 * them. New proplist code from WINGs provide a better integration with the
 * other data types from WINGs, not to mention that the proplist code in WINGs
 * is actively maintained while the old libPropList is dead.
 *
 */


#ifndef _PROPLIST_COMPAT_H_
#define _PROPLIST_COMPAT_H_

#include <WINGs/WUtil.h>


typedef WMPropList* proplist_t;


#ifndef YES
#define YES True
#endif

#ifndef NO
#define NO False
#endif


#define PLSetCaseSensitive(c) WMPLSetCaseSensitive(c)

#define PLMakeString(bytes) WMCreatePLString(bytes)
#define PLMakeData(bytes, length) WMCreatePLDataWithBytes(bytes, length)
#define PLMakeArrayFromElements WMCreatePLArray
#define PLMakeDictionaryFromEntries WMCreatePLDictionary

#define PLRetain(pl) WMRetainPropList(pl)
#define PLRelease(pl) WMReleasePropList(pl)

#define PLInsertArrayElement(array, pl, pos) WMInsertInPLArray(array, pos, pl)
#define PLAppendArrayElement(array, pl) WMAddToPLArray(array, pl)
#define PLRemoveArrayElement(array, pos) WMDeleteFromPLArray(array, pos)
#define PLInsertDictionaryEntry(dict, key, value) WMPutInPLDictionary(dict, key, value)
#define PLRemoveDictionaryEntry(dict, key) WMRemoveFromPLDictionary(dict, key)
#define PLMergeDictionaries(dest, source) WMMergePLDictionaries(dest, source, False)

#define PLGetNumberOfElements(pl) WMGetPropListItemCount(pl)

#define PLIsString(pl) WMIsPLString(pl)
#define PLIsData(pl) WMIsPLData(pl)
#define PLIsArray(pl) WMIsPLArray(pl)
#define PLIsDictionary(pl) WMIsPLDictionary(pl)
#define PLIsSimple(pl) (WMIsPLString(pl) || WMIsPLData(pl))
#define PLIsCompound(pl) (WMIsPLArray(pl) || WMIsPLDictionary(pl))
#define PLIsEqual(pl1, pl2) WMIsPropListEqualTo(pl1, pl2)

#define PLGetString(pl) WMGetFromPLString(pl)
#define PLGetDataBytes(pl) WMGetPLDataBytes(pl)
#define PLGetDataLength(pl) WMGetPLDataLength(pl)
#define PLGetArrayElement(pl, index) WMGetFromPLArray(pl, index)
#define PLGetDictionaryEntry(pl, key) WMGetFromPLDictionary(pl, key)
#define PLGetAllDictionaryKeys(pl) WMGetPLDictionaryKeys(pl)

#define PLShallowCopy(pl) WMShallowCopyPropList(pl)
#define PLDeepCopy(pl) WMDeepCopyPropList(pl)

#define PLGetProplistWithDescription(desc) WMCreatePropListFromDescription(desc)
#define PLGetDescriptionIndent(pl, level) WMGetPropListDescription(pl, True)
#define PLGetDescription(pl) WMGetPropListDescription(pl, False)
#define PLGetStringDescription(pl) WMGetPropListDescription(pl, False)
#define PLGetDataDescription(pl) WMGetPropListDescription(pl, False)

#define PLGetProplistWithPath(file) WMReadPropListFromFile(file)
#define PLSave(pl, file, atm) WMWritePropListToFile(pl, file, atm)


/* Unsupported functions. Do not ask for them. They're evil :P */
#define PLSetStringCmpHook(fn) error_PLSetStringCmpHook_is_not_supported
#define PLDeepSynchronize(pl) error_PLDeepSynchronize_is_not_supported
#define PLSynchronize(pl) error_PLSynchronize_is_not_supported
#define PLShallowSynchronize(pl) error_PLShallowSynchronize_is_not_supported
#define PLSetFilename(pl, filename) error_PLSetFilename_is_not_supported
#define PLGetFilename(pl, filename) error_PLGetFilename_is_not_supported
#define PLGetContainer(pl) error_PLGetContainer_is_not_supported

#define PLGetDomainNames error_PLGetDomainNames_is_not_supported
#define PLGetDomain(name) error_PLGetDomain_is_not_supported
#define PLSetDomain(name, value, kickme) error_PLSetDomain_is_not_supported
#define PLDeleteDomain(name, kickme) error_PLDeleteDomain_is_not_supported
#define PLRegister(name, callback) error_PLRegister_is_not_supported
#define PLUnregister(name) error_PLUnregister_is_not_supported


#endif
