#ifndef _COLLECTION_SYNC_H
#define _COLLECTION_SYNC_H

#include <string>
#include <set>
#include <vector>

#include <musicbrainz4/Query.h>

class CCollectionSync
{
public:
	CCollectionSync(const std::string& User,
		const std::string& Password,
		const std::string& CollectionID,
		const std::string& Path);

private:
	std::string m_User;
	std::string m_Password;
	std::string m_CollectionID;
	std::string m_Path;
	MusicBrainz4::CQuery m_Query;

	std::set<std::string> m_LocalReleases;
	std::set<std::string> m_CollectionReleases;
	std::vector<std::string> m_ToAdd;
	std::vector<std::string> m_ToDelete;

	void BuildFileList();
	void ScanDir(const std::string& Dir);
	std::string GetReleaseID(const std::string& FileName);
	void GetCollectionContents();
	void BuildLists();
	void PerformDeletions();
	void PerformAdds();
};

#endif
