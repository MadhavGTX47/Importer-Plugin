// Fill out your copyright notice in the Description page of Project Settings.


#include "MyFactory.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Editor/UnrealEd/Public/Editor.h"

#include "Engine/VolumeTexture.h"

#include "Engine/Texture.h"

#define LOCTEXT_NAMESPACE "assn5LoadFactory"

UMyFactory::UMyFactory(const FObjectInitializer& ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = false;
	bEditorImport = true;   
	bText = false;  // text src

	SupportedClass = UVolumeTexture::StaticClass();

	Formats.Add(TEXT("mha;mha format"));
	Formats.Add(TEXT("mhd;mhd format"));
}

UObject* UMyFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UVolumeTexture* myTexture = NewObject<UVolumeTexture>(InParent, InName, Flags);	//create blank texture
	UTexture2D* myTexture2d = NewObject<UTexture2D>(InParent, InName, Flags);	//create blank texture
	UObject* Object = nullptr;
	const FString Filepath = FPaths::ConvertRelativePathToFull(Filename);
	const FString myDirectory = FPaths::GetPath(Filepath);	//get directory that this full file path is stored in for use later with binary directory
	TArray <FString> fileContents;
	FFileHelper::LoadFileToStringArray(fileContents, *Filepath);//load mha or mhd text file to array
	int32 arraySize = fileContents.Num();

	//for storing some values 
	int32 myNDims = -1;
	int32 dimSizeX = -1;
	int32 dimSizeY = -1;
	int32 dimSizeZ = -1;
	int32 myNumChannels = -1;
	FString elementType = "error";

	//reading over and logging array to confirm if this step worked
	//also, taking actions based on contents of each file line


	int count = 0;


	FScopedSlowTask ImportTask(fileContents.Num(), LOCTEXT("Importing", "Importing "));
	ImportTask.MakeDialog(true);
	

	for (FString myString : fileContents)
	{
		ImportTask.EnterProgressFrame();
		if (ImportTask.ShouldCancel())
			break;


		if (count == 0)
		{	
			if (!myString.Equals(TEXT("ObjectType = Image"), ESearchCase::IgnoreCase)){
				return nullptr;
			}
		}

		else if (count == (arraySize - 1))
		{
	

			//has to contain an element data file then it is a .mha file
			if (!(myString.Contains(TEXT("ElementDataFile"))))
			{
				return nullptr;

			}
			else
			{
				
				FString fileNameRaw = myString.RightChop(18);
				FString fullPathRaw = myDirectory + FString("/") + fileNameRaw;

				TArray <uint8> rawContents;
				bool worked = FFileHelper::LoadFileToArray(rawContents, *fullPathRaw);

				if (!worked)
				{
					UE_LOG(LogTemp, Warning, TEXT("Loading, Check formatting of your mhd/mha file."));
					return nullptr;
				}

				//get pointer to the tarray.
				uint8* rawPointer = rawContents.GetData();
				if (myNDims == 2 && elementType != "MET_FLOAT") {
					myTexture2d->Source.Init(dimSizeX, dimSizeY, 0, 1, TSF_G8, rawPointer);
					myTexture2d->SRGB = 1;
					myTexture2d->UpdateResource();
				}
				if (elementType == "MET_FLOAT")
					myTexture->Source.Init(dimSizeX, dimSizeY, dimSizeZ, 1, TSF_RGBA16F, rawPointer);
				//Now set values of the uvolume texture accordingly. 
				myTexture->Source.Init(dimSizeX, dimSizeY, dimSizeZ, 1, TSF_G8, rawPointer);
				
				
				//set srgb to 0
				myTexture->SRGB = 1;
				//update texture
				myTexture->UpdateResource();

			}
		}
		else
		{
			
			//check if NDims are correct
			if (myString.Contains(TEXT("NDims")))
			{
				//not 3 dimensions
				if (!myString.Equals(TEXT("NDims = 3"), ESearchCase::IgnoreCase))
				{
					UE_LOG(LogTemp, Warning, TEXT("Value error for NDims"));
					return nullptr;
				}
				else if (myString.Equals(TEXT("NDims = 2"), ESearchCase::IgnoreCase))
				{
					//set NDims based on file
					myNDims = 2;

				}
				else
				{
					//set NDims based on file
					myNDims = 3;
					
				}
			}
			//if not 3 positive values, output error
			else if (myString.Contains(TEXT("DimSize =")))
			{
				
				FString myDimSizes = myString.RightChop(10);
				//making array for storing them
				TArray <FString> myDimSizesArray;
				//parse into array useing whitespace as seperator
				myDimSizes.ParseIntoArrayWS(myDimSizesArray);

				//first check if array is not right size
				if (!((myDimSizesArray.Num() == 3) || (myDimSizesArray.Num() == 2)))
				{
					UE_LOG(LogTemp, Warning, TEXT("My dim sizes have the incorrect number of dimensions"));
					return nullptr;
				}

				//loop over the results
				int countDim = 0;
				for (FString dimSize : myDimSizesArray)
				{
					int32 tempInt = FCString::Atoi(*dimSize);
					switch (countDim)
					{
					case 0:
						if (tempInt <= 0)
						{
								return nullptr;
						}
						else
							dimSizeX = tempInt;
						break;
					case 1:
						if (tempInt <= 0)
						{
								return nullptr;
						}
						else
							dimSizeY = tempInt;
						break;

					case 2:
						if (tempInt <= 0)
						{
								return nullptr;
						}
						else
							dimSizeZ = tempInt;
						break;
					default:
						return nullptr;
						break;
					}
					countDim++;
				}


			}
			//number of channels
			else if (myString.Contains(TEXT("ElementNumberOfChannels")))
			{
				FString nChannels = myString.RightChop(25);
				int32 tempInt = FCString::Atoi(*nChannels);

				if (tempInt == 1)
				{
					myNumChannels = tempInt;
				}
				else
				{
					return nullptr;
				}

				
			}

			//check element type.
			else if (myString.Contains(TEXT("ElementType")))
			{
				
				if (myString.Equals(TEXT("ElementType = MET_UCHAR"), ESearchCase::IgnoreCase))
				{
					elementType = "MET_UCHAR";
				}
				if (myString.Equals(TEXT("ElementType = MET_FLOAT"), ESearchCase::IgnoreCase))
				{
					elementType = "MET_FLOAT";
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Datatype for color elements is incorrect"));
					return nullptr;
				}

				
			}
			else
			{}

		}
		count++;


		ImportTask.EnterProgressFrame(1);
	}

	if(myNDims==2)
		Object = myTexture2d;
	else
		Object = myTexture;
	


	return Object;
}