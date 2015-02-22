#pragma once

#include "wlmdbim.h"
#include "wlds.h"

class WlmDataSourceDB : public WlmDataSource
{
private:
  OFBool enableRejectionOfIncompleteWlFiles;
  WlmDBInteractionManager *dbInteractionManager;

protected:
  /** This function takes care of handling a certain non-sequence element whithin
   *  the structure of a certain result dataset. This function assumes that all
   *  elements in the result dataset are supported. In detail, a value for the
   *  current element with regard to the currently processed matching record will
   *  be requested from the fileSystemInteractionManager, and this value will be
   *  set in the element.
   *  @param element Pointer to the currently processed element.
   *  @param idx     Index of the matching record (identifies this record).
   */
    void HandleNonSequenceElementInResultDataset( DcmElement *element, unsigned long idx );

  /** This function takes care of handling a certain sequence element within the structure
   *  of a certain result dataset. On the basis of the matching record from the data source,
   *  this function will add items and values for all elements in these items to the current
   *  sequence element in the result dataset. This function assumes that all elements in the
   *  result dataset are supported. In case the current sequence element contains no items or
   *  more than one item, this element will be left unchanged.
   *  @param element Pointer to the currently processed element.
   *  @param idx     Index of the matching record (identifies this record).
   */
  void HandleSequenceElementInResultDataset( DcmElement *element, unsigned long idx );

  /** This function performs a check on two attributes in the given dataset. At two different places
   *  in the definition of the DICOM worklist management service, a description attribute and a code
   *  sequence attribute with a return type of 1C are mentioned, and the condition specifies that
   *  either the description attribute or the code sequence attribute or both shall be supported by
   *  an SCP. (I am talking about RequestedProcedureDescription vs. RequestedProcedureCodeSequence
   *  and ScheduledProcedureStepDescription vs. ScheduledProtocolCodeSequence.) In both cases, this
   *  implementation actually supports both, the description _and_ the code sequence attributes.
   *  In cases where the description attribute is actually empty or the code sequence attribute
   *  is actually empty or contains exactly one item with an empty CodeValue and an empty
   *  CodingSchemeDesignator, we want to remove the empty attribute from the dataset. This is what
   *  this function does. (Please note, that this function will always only delete one of the two,
   *  and this function will start checking the sequence attribute.
   *  @param dataset Dataset in which the consistency of the two attributes shall be checked.
   *  @param descriptionTagKey DcmTagKey of the description attribute which shall be checked.
   *  @param codeSequenceTagKey DcmTagKey of the codeSequence attribute which shall be checked.
   */
  void HandleExistentButEmptyDescriptionAndCodeSequenceAttributes( DcmItem *dataset, const DcmTagKey &descriptionTagKey, const DcmTagKey &codeSequenceTagKey );

  /** This function performs a check on a sequence attribute in the given dataset. At two different places
   *  in the definition of the DICOM worklist management service, a sequence attribute with a return type
   *  of 2 is mentioned containing two 1C attributes in its item; the condition of the two 1C attributes
   *  specifies that in case a sequence item is present, then these two attributes must be existent and
   *  must contain a value. (I am talking about ReferencedStudySequence and ReferencedPatientSequence.)
   *  In cases where the sequence attribute contains exactly one item with an empty ReferencedSOPClass
   *  and an empty ReferencedSOPInstance, we want to remove the item from the sequence. This is what
   *  this function does.
   *  @param dataset Dataset in which the consistency of the sequence attribute shall be checked.
   *  @param sequenceTagKey DcmTagKey of the sequence attribute which shall be checked.
   */
  void HandleExistentButEmptyReferencedStudyOrPatientSequenceAttributes( DcmDataset *dataset, const DcmTagKey &sequenceTagKey );

public:
  WlmDataSourceDB(void);
  virtual ~WlmDataSourceDB(void);

  virtual void SetEnableRejectionOfIncompleteWlFiles( OFBool /*value*/ );

  /** Connects to the data source.
   * @return Indicates if the connection was established succesfully.
   */
  virtual OFCondition ConnectToDataSource();

  /** Disconnects from the data source.
   * @return Indicates if the disconnection was completed succesfully.
   */
  virtual OFCondition DisconnectFromDataSource();

  /** Checks if the called application entity title is supported. This function expects
   *  that the called application entity title was made available for this instance through
   *  WlmDataSource::SetCalledApplicationEntityTitle(). If this is not the case, OFFalse
   *  will be returned.
   *  @return OFTrue, if the called application entity title is supported;
   *          OFFalse, if the called application entity title is not supported or it is not given.
   */
  virtual OFBool IsCalledApplicationEntityTitleSupported();

  /** This function determines the records from the worklist files which match
   *  the given search mask and returns the number of matching records. Also,
   *  this function will store the matching records in memory in the array
   *  member variable matchingRecords.
   *  @param searchMask - [in] The search mask.
   *  @param matchingRecords - [out] matching record **&.
   *  @return Number of matching records.
   */
  unsigned long DetermineMatchingRecords( DcmDataset *searchMask, DcmDataset **&matchingRecords );

  /** Based on the search mask which was passed, this function determines all the records
   *  in the database which match the values of matching key attributes in the search mask.
   *  For each matching record, a DcmDataset structure is generated which will later be
   *  returned to the SCU as a result of query. The DcmDataset structures for all matching
   *  records will be stored in the protected member variable matchingDatasets.
   *  @param findRequestIdentifiers Contains the search mask.
   *  @return A WlmDataSourceStatusType value denoting the following:
   *          WLM_SUCCESS: No matching records found;
   *          WLM_PENDING: Matching records found, all return keys supported by this application;
   *          WLM_PENDING_WARNING: Matching records found, not all return keys supported by this application;
   *          WLM_FAILED_IDENTIFIER_DOES_NOT_MATCH_SOP_CLASS: Error in the search mask encountered.
   */
  virtual WlmDataSourceStatusType StartFindRequest( DcmDataset &findRequestIdentifiers );

  /** This function will return the next dataset that matches the given search mask, if
   *  there is one more resulting dataset to return. In such a case, rstatus will be set
   *  to WLM_PENDING or WLM_PENDING_WARNING, depending on if an unsupported key attribute
   *  was encountered in the search mask or not. If there are no more datasets that match
   *  the search mask, this function will return an empty dataset and WLM_SUCCESS in rstatus.
   *  @param rStatus A value of type WlmDataSourceStatusType that can be used to
   *                 decide if there are still elements that have to be returned.
   *  @return The next dataset that matches the given search mask, or an empty dataset if
   *          there are no more matching datasets in the database.
   */
  virtual DcmDataset *NextFindResponse( WlmDataSourceStatusType &rStatus );
};
