#pragma once

void commitToDB();
void generateImageStoreDirectory(DcmDataset **imageDataSet, OFString& subdirectoryName, OFString& imageManageNumber);
bool insertImage(DcmDataset *imageDataSet, OFString& imageManageNumber, OFString& outputDirectory, OFString& relateFilePathName);