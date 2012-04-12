#include "CollectionSync.h"

#include <iostream>
#include <sstream>

#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#include <FLAC++/metadata.h>

#include <musicbrainz4/Collection.h>
#include <musicbrainz4/HTTPFetch.h>
#include <musicbrainz4/ReleaseList.h>
#include <musicbrainz4/Release.h>

CCollectionSync::CCollectionSync(const std::string& User,
	const std::string& Password,
	const std::string& CollectionID,
	const std::string& Path)
:	m_User(User),
	m_Password(Password),
	m_CollectionID(CollectionID),
	m_Path(Path),
	m_Query("mb-collection-sync-1.0")
{
	m_Query.SetUserName(m_User);
	m_Query.SetPassword(m_Password);

	GetCollectionContents();

/*
	std::vector<std::string> ToDelete(m_CollectionReleases.begin(),m_CollectionReleases.end());

	try
	{
		m_Query.DeleteCollectionEntries(m_CollectionID,ToDelete);
	}

	catch (MusicBrainz4::CConnectionError& Error)
	{
		std::cout << "Connection Exception: '" << Error.what() << "'" << std::endl;
		std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
		std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
		std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
	}

	catch (MusicBrainz4::CTimeoutError& Error)
	{
		std::cout << "Timeout Exception: '" << Error.what() << "'" << std::endl;
		std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
		std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
		std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
	}

	catch (MusicBrainz4::CAuthenticationError& Error)
	{
		std::cout << "Authentication Exception: '" << Error.what() << "'" << std::endl;
		std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
		std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
		std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
	}

	catch (MusicBrainz4::CFetchError& Error)
	{
		std::cout << "Fetch Exception: '" << Error.what() << "'" << std::endl;
		std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
		std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
		std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
	}

	catch (MusicBrainz4::CRequestError& Error)
	{
		std::cout << "Request Exception: '" << Error.what() << "'" << std::endl;
		std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
		std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
		std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
	}

	catch (MusicBrainz4::CResourceNotFoundError& Error)
	{
		std::cout << "ResourceNotFound Exception: '" << Error.what() << "'" << std::endl;
		std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
		std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
		std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
	}
	GetCollectionContents();
*/

	BuildFileList();
	BuildLists();
	PerformDeletions();
	PerformAdds();
}

void CCollectionSync::BuildFileList()
{
	ScanDir(m_Path);
}

void CCollectionSync::ScanDir(const std::string& Directory)
{
	std::cout << "Scanning: " << Directory << std::endl;

	DIR *Dir=opendir(Directory.c_str());
	if (Dir)
	{
		struct dirent *Entry=0;

		do
		{
			Entry=readdir(Dir);
			if (Entry)
			{
				std::string Name(Entry->d_name);

				if (Name!="." && Name!="..")
				{
					if (DT_DIR==Entry->d_type)
					{
						ScanDir(Directory+"/"+Name);
					}
					else if (DT_REG==Entry->d_type)
					{
						if (Name.substr(Name.length()-5,5)==".flac")
						{
							std::string ReleaseID=GetReleaseID(Directory+"/"+Name);

							if (!ReleaseID.empty())
								m_LocalReleases.insert(ReleaseID);
						}
					}
				}
			}
		} while (Entry);

		closedir(Dir);
	}
}

void CCollectionSync::GetCollectionContents()
{
	m_CollectionReleases.clear();

	std::cout << "Reading collection" << std::endl;

	int Offset=0;
	int NumItems=0;

	do
	{
		try
		{
			std::map<std::string,std::string> Params;
			std::stringstream os;
			os << Offset;
			Params["offset"]=os.str();
			Params["limit"]="100";

			MusicBrainz4::CMetadata Metadata=m_Query.Query("collection",m_CollectionID,"releases",Params);

			if (Metadata.Collection())
			{
				MusicBrainz4::CCollection *Collection=Metadata.Collection();
				if (Collection->ReleaseList())
				{
					MusicBrainz4::CReleaseList *ReleaseList=Collection->ReleaseList();

					NumItems=ReleaseList->Count();
					Offset+=ReleaseList->NumItems();

					for (int count=0;count<ReleaseList->NumItems();count++)
					{
						MusicBrainz4::CRelease *Release=ReleaseList->Item(count);

						if (!Release->ID().empty())
						{
							if (Release->ID().find("fb690060")!=std::string::npos)
							{
								std::cout << "Found release ID: '" << Release->ID() << "'" << std::endl;
								sleep(5);
							}

							m_CollectionReleases.insert(Release->ID());
						}
					}
				}
			}
		}

		catch (MusicBrainz4::CConnectionError& Error)
		{
			std::cout << "Connection Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CTimeoutError& Error)
		{
			std::cout << "Timeout Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CAuthenticationError& Error)
		{
			std::cout << "Authentication Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CFetchError& Error)
		{
			std::cout << "Fetch Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CRequestError& Error)
		{
			std::cout << "Request Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CResourceNotFoundError& Error)
		{
			std::cout << "ResourceNotFound Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}
	} while ((int)m_CollectionReleases.size()!=NumItems);

	std::cout << "Size is " << m_CollectionReleases.size() << std::endl;
}

void CCollectionSync::BuildLists()
{
	std::cout << "Building lists" << std::endl;

	for (std::set<std::string>::const_iterator ThisItem=m_LocalReleases.begin();ThisItem!=m_LocalReleases.end();++ThisItem)
	{
		std::string Release=*ThisItem;
		if (m_CollectionReleases.end()==m_CollectionReleases.find(Release))
			m_ToAdd.push_back(Release);
	}

	for (std::set<std::string>::const_iterator ThisItem=m_CollectionReleases.begin();ThisItem!=m_CollectionReleases.end();++ThisItem)
	{
		std::string Release=*ThisItem;
		if (m_LocalReleases.end()==m_LocalReleases.find(Release))
			m_ToDelete.push_back(Release);
	}
}

void CCollectionSync::PerformDeletions()
{
	if (m_ToDelete.empty())
		std::cout << "Nothing to delete" << std::endl;
	else
	{
		std::cout << "Deleting: " << m_ToDelete.size() << std::endl;

//		for (std::vector<std::string>::const_iterator ThisItem=m_ToDelete.begin();ThisItem!=m_ToDelete.end();++ThisItem)
//			std::cout << "Deleting: '" << *ThisItem << "'" << std::endl;

		try
		{
			m_Query.DeleteCollectionEntries(m_CollectionID,m_ToDelete);
		}

		catch (MusicBrainz4::CConnectionError& Error)
		{
			std::cout << "Connection Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CTimeoutError& Error)
		{
			std::cout << "Timeout Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CAuthenticationError& Error)
		{
			std::cout << "Authentication Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CFetchError& Error)
		{
			std::cout << "Fetch Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CRequestError& Error)
		{
			std::cout << "Request Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CResourceNotFoundError& Error)
		{
			std::cout << "ResourceNotFound Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}
	}
}

void CCollectionSync::PerformAdds()
{
	if (m_ToAdd.empty())
		std::cout << "Nothing to add" << std::endl;
	else
	{
		std::vector<std::string> ToAdd=m_ToAdd;
//		while (ToAdd.size()<50)
//			ToAdd.push_back(m_ToAdd[ToAdd.size()]);
			
		std::cout << "Adding: " << ToAdd.size() << std::endl;
//		for (std::vector<std::string>::const_iterator ThisItem=ToAdd.begin();ThisItem!=ToAdd.end();++ThisItem)
//			std::cout << "Adding: '" << *ThisItem << "'" << std::endl;

		try
		{
			m_Query.AddCollectionEntries(m_CollectionID,ToAdd);
		}

		catch (MusicBrainz4::CConnectionError& Error)
		{
			std::cout << "Connection Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CTimeoutError& Error)
		{
			std::cout << "Timeout Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CAuthenticationError& Error)
		{
			std::cout << "Authentication Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CFetchError& Error)
		{
			std::cout << "Fetch Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CRequestError& Error)
		{
			std::cout << "Request Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}

		catch (MusicBrainz4::CResourceNotFoundError& Error)
		{
			std::cout << "ResourceNotFound Exception: '" << Error.what() << "'" << std::endl;
			std::cout << "LastResult: " << m_Query.LastResult() << std::endl;
			std::cout << "LastHTTPCode: " << m_Query.LastHTTPCode() << std::endl;
			std::cout << "LastErrorMessage: " << m_Query.LastErrorMessage() << std::endl;
		}
	}
}

std::string CCollectionSync::GetReleaseID(const std::string& FileName)
{
	std::string Ret;

	FLAC::Metadata::Chain Chain;
	if (Chain.read(FileName.c_str()))
	{
		if (Chain.is_valid())
		{
			FLAC::Metadata::Iterator Iterator;

			Iterator.init(Chain);

			if (Iterator.is_valid())
			{
				do
				{
					switch (Iterator.get_block_type())
					{
						case FLAC__METADATA_TYPE_STREAMINFO:
							break;

						case FLAC__METADATA_TYPE_PADDING:
							break;

						case FLAC__METADATA_TYPE_APPLICATION:
							break;

						case FLAC__METADATA_TYPE_SEEKTABLE:
							break;

						case FLAC__METADATA_TYPE_VORBIS_COMMENT:
						{
							FLAC::Metadata::VorbisComment *TagBlock=(FLAC::Metadata::VorbisComment *)Iterator.get_block();

							if (TagBlock->is_valid())
							{
								for (unsigned count=0;count<TagBlock->get_num_comments();count++)
								{
									FLAC::Metadata::VorbisComment::Entry Entry=TagBlock->get_comment(count);

									std::string Name(Entry.get_field_name(),Entry.get_field_name_length());
									std::string Value(Entry.get_field_value(),Entry.get_field_value_length());

									if (Name=="MUSICBRAINZ_ALBUMID")
										Ret=Value;
								}
							}

							delete TagBlock;

							break;
						}

						case FLAC__METADATA_TYPE_CUESHEET:
							break;

						case FLAC__METADATA_TYPE_UNDEFINED:
							break;

#ifdef FLAC_API_VERSION_CURRENT
						case FLAC__METADATA_TYPE_PICTURE:
							break;
#endif

					}
				} while (Iterator.next() && Ret.empty());
			}
		}
	}

	return Ret;
}
