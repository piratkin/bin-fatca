//---------------------------------------------------------------------------

#include <vcl.h>
#include <tchar.h>
#include <stdio.h>
//#include <SysUtils.hpp>
#include "set_user_name_form.h"
#pragma hdrstop
#include "main_form.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
Tfatca *fatca;

enum
{
	FIELD_CONSTANT  = 1,
	FIELD_LINKED    = 2,
	FIELD_NOT_FOUND = 4,
	FIELD_AUTO_GEN  = 8
};
//---------------------------------------------------------------------------
__fastcall Tfatca::Tfatca(TComponent* Owner)
	: TForm(Owner), fatca_state(NULL), meta_state(NULL)
{
}
//---------------------------------------------------------------------------
void split(TStringList *slist, AnsiString &str, char delim)
{
  char *fst = str.c_str();
  while(*fst!=0)
  {
	char *lst = strchr(fst, delim);
	if(lst==NULL)
	{
	  slist->Add(Trim(fst));
	  break;
	}
	*lst = 0;
	slist->Add(fst);
	*lst = delim;
	fst = lst + 1;
  }
}
//--------------------------------------------------------
UnicodeString Tfatca::PrintVersionStringInfo(const TCHAR *fileName)
{
	UnicodeString version_info = "";

	PLONG infoBuffer;  // �����, ���� ����� ������ ����������
	DWORD infoSize;    // � ��� ������

	struct LANGANDCODEPAGE { // ��������� ��� ��������� ������� ������� �� ������ ���������� �������� �����
		WORD wLanguage;
		WORD wCodePage;
	} *pLangCodePage;

	// ����� ����������, �������� ������� ����� �����������
	const TCHAR *paramNames[] = {
			_T("FileDescription"),
			_T("CompanyName"),
			_T("FileVersion"),
			_T("InternalName"),
			_T("LegalCopyright"),
			_T("LegalTradeMarks"),
			_T("OriginalFilename"),
			_T("ProductName"),
			_T("ProductVersion"),
			_T("Comments"),
			_T("Author")
	};

	TCHAR paramNameBuf[256]; // ����� ��������� ��� ���������
	TCHAR *paramValue;       // ����� ����� ��������� �� �������� ���������, ������� ��� ������ ������� VerQueryValue
	UINT paramSz;            // ����� ����� ����� �������� ���������, ������� ��� ������ ������� VerQueryValue

	// �������� ������ ���������� � ������ �����
	infoSize = GetFileVersionInfoSize(fileName, NULL);
	if ( infoSize > 0 )
	{
		// �������� ������
		infoBuffer = (PLONG) malloc(infoSize);

        // �������� ����������
        if ( 0 != GetFileVersionInfo(fileName, NULL, infoSize, infoBuffer) )
        {
            // ���������� ��������� ������� � ���� "\StringFileInfo\�������_��������\���_���������
            // �.�. �� �� ����� ������� ������� � ����� ����������� (�������_��������) �������� ������� � �����,
            // �� ����� ���������� �� ���

            UINT cpSz;
            // �������� ������ ������� �������
            if ( VerQueryValue(infoBuffer,                      // ��� �����, ���������� ����������
                               _T("\\VarFileInfo\\Translation"),// ����������� ��������� ����������
                               (LPVOID*) &pLangCodePage,        // ���� ������� ������ ��� ��������� �� ������ ������������ ��� ������
                               &cpSz) )                         // � ���� - ������ ���� ������
            {
				// ���������� ��� ������� ��������
				for (int cpIdx = 0; cpIdx < (int)(cpSz/sizeof(struct LANGANDCODEPAGE)); cpIdx++ )
				{
					// ���������� ����� ����������
					for (int paramIdx = 0; paramIdx < sizeof(paramNames)/sizeof(char*); paramIdx++)
					{
						// ��������� ��� ��������� ( \StringFileInfo\�������_��������\���_��������� )
						swprintf(	paramNameBuf, _T("\\StringFileInfo\\%04x%04x\\%s"),
									pLangCodePage[cpIdx].wLanguage,  // ��, ��� ����� ������� ������ ���
									pLangCodePage[cpIdx].wCodePage,  // �����-�� ��������� ������� ��������
									paramNames[paramIdx]);

						// �������� �������� ���������
						if ( VerQueryValue(infoBuffer, paramNameBuf, (LPVOID*)&paramValue, &paramSz)) {
							if (UnicodeString(paramNames[paramIdx]) == "FileVersion") {
								version_info = UnicodeString(paramValue);
							}
						}
					}
				}
			}
		}
		free(infoBuffer);
	}
	return version_info;
}
//---------------------------------------------------------------------------
UnicodeString config;  //����� ��������������� �������, ���. ��� �������� ���������!
UnicodeString auto_id, /*auto_date, auto_time,*/ auto_nodename, auto_timestamp;
void __fastcall Tfatca::FormCreate(TObject *Sender)
{
	if (FileExists("fatca.txml"))  xml->LoadFromFile("fatca.txml");
	if (FileExists("config.txml")) cnf->LoadFromFile("config.txml");
	if (FileExists("model.txml"))  mdl->LoadFromFile("model.txml");
	if (FileExists("meta.txml"))   mtf->LoadFromFile("meta.txml");
	xml->Active = true;
	cnf->Active = true;
	mdl->Active = true;
	mtf->Active = true;
	xml_text->Text = xml->XML->Text;
	config = cnf->XML->Text;

	//���������� ID!
	srand(time(NULL));
	auto_id  = IntToHex(rand(), 4);
	auto_id += IntToHex(rand(), 4);
	auto_id += "-";
	auto_id += IntToHex(rand(), 4);
	auto_id += "-";
	auto_id += IntToHex(rand(), 4);
	auto_id += "-";
	auto_id += IntToHex(rand(), 4);
	auto_id += "-";
	auto_id += IntToHex(rand(), 4);
	auto_id += IntToHex(rand(), 4);
	auto_id += IntToHex(rand(), 4);
	auto_id = auto_id.LowerCase();

	//������� ���� � �����
	ShortDateFormat = "yyyy-mm-dd";
	ShortTimeFormat = "HH:nn:ss";
	auto_timestamp = Date().DateString() + "T" + Time().TimeString();

	//���������� ������ ���� ��� �������� � ����� �� ������ 3-� �������
	UnicodeString version = xml->DocumentElement->GetAttribute("version");
	if (version.Length() == 3 && version.Pos(".")) {
		fatca->Caption = fatca->Caption + " - v" + version;
	}

	if (cnf->DocumentElement->ChildNodes->FindNode("Config")->ChildNodes->FindNode("Hint")->GetText() == 1) {
		menu_hint->Checked   = true;
		fatca_list->ShowHint = true;
		meta_list->ShowHint  = true;
	}

	fatca_xml_load();
	meta_xml_load();

	//������� ������ �������
	fatca_list->DeleteRow(1);
	meta_list->DeleteRow(1);
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::history_create() {

	char *error_message = 0;
	AnsiString sql_req;

	if (sqlite3_open("arhive.jnl", &db) == SQLITE_OK) {
		sql_req = "CREATE TABLE HISTORY                                          \
				   (                                                             \
					   ID        INTEGER CONSTRAINT id PRIMARY KEY AUTOINCREMENT,\
					   TIMESTAMP STRING,                                         \
					   TYPE      STRING,                                         \
					   REFID     STRING,                                         \
					   NAME      STRING,                                         \
					   HIDE      INTEGER DEFAULT NULL,                           \
					   FILE      BLOB                                            \
				   );";
		if (sqlite3_exec(db, sql_req.c_str(), 0, 0, &error_message) > SQLITE_ERROR) ShowMessage(error_message);
	} else {
		UnicodeString file_name = ExtractFilePath(Application->ExeName) + "arhive.jnl";

		if (MessageDlg("�� ������� ������� ������! �������\n"
					   "������� ���� \"arhive.jnl\" � ����������\n"
					   "���������. ��������, �� ������������\n"
					   "������ \"���������\", ���������� �������\n"
					   "���� ������?",
					   mtWarning,
					   TMsgDlgButtons() << mbYes << mbNo, 0) != mrYes) {
			DeleteFile(file_name);
		}
		exit(0);
	}
	sqlite3_free(error_message);
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::history_meta_save()
{
	sqlite3_stmt *stmt;
	char *error_message = 0;
	AnsiString sql_req;

	if (db) {
		sql_req  = "INSERT INTO HISTORY                   \
					(                                     \
						TIMESTAMP,                        \
						TYPE,                             \
						REFID,                            \
						NAME,                             \
						FILE                              \
					) VALUES (                            \
						\'" + auto_timestamp + "Z" + "\', \
						\'META\',                         \
						\'" + auto_nodename + "\',        \
						\'" + user_name + "\',            \
						?                                 \
					);";

		if (sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0) != SQLITE_OK) {
			ShowMessage(error_message);
		} else {
			AnsiString buffer = mtf->XML->Text;
			if (sqlite3_bind_blob(stmt, 1, buffer.c_str(), buffer.Length(), SQLITE_STATIC) == SQLITE_OK) {
				if (sqlite3_step(stmt) != SQLITE_DONE) {
					ShowMessage(error_message);
				}
			} else {
				ShowMessage(error_message);
			}
		}
	} else {
		ShowMessage("��������� ������� ������!");
	}

	sqlite3_free(error_message);
	sqlite3_finalize(stmt);
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::history_xml_save()
{
	sqlite3_stmt *stmt;
	char *error_message = 0;
	AnsiString sql_req;

	if (db) {
		sql_req  = "INSERT INTO HISTORY             \
					(                               \
						TIMESTAMP,                  \
						TYPE,                       \
						REFID,                      \
						NAME,                       \
						FILE                        \
					) VALUES (                      \
						\'" + auto_timestamp + "\', \
						\'DATA\',                   \
						\'" + auto_nodename + "\',  \
						\'" + user_name + "\',      \
						?                           \
					);";

		if (sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0) != SQLITE_OK) {
			ShowMessage(error_message);
		} else {
			AnsiString buffer = xml->XML->Text;
			if (sqlite3_bind_blob(stmt, 1, buffer.c_str(), buffer.Length(), SQLITE_STATIC) == SQLITE_OK) {
				if (sqlite3_step(stmt) != SQLITE_DONE) {
					ShowMessage(error_message);
				}
			} else {
				ShowMessage(error_message);
			}
		}
	} else {
		ShowMessage("��������� ������� ������!");
	}

	sqlite3_free(error_message);
	sqlite3_finalize(stmt);
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::FormShow(TObject *Sender)
{
	history_create();
	history_load();
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::history_load()
{
	sqlite3_stmt *stmt;
	char *error_message = 0;
	AnsiString sql_req;

	if (db) {
		sql_req = "SELECT ID, TIMESTAMP, TYPE, REFID, NAME FROM HISTORY WHERE HIDE IS NULL;";
		int rc = sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0);
		if (rc != SQLITE_OK) {
			ShowMessage(error_message);
		} else {
			//���������� ������� ��������
			journal_list->Cells[0][0] = "ID";
			journal_list->Cells[1][0] = "TIMESTAMP";
			journal_list->Cells[2][0] = "TYPE";
			journal_list->Cells[3][0] = "REFID";
			journal_list->Cells[4][0] = "NAME";

			int row_index = 1;
			while (1) {
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					 journal_list->Cells[0][row_index] = (char *)sqlite3_column_text(stmt, 0);
					 journal_list->Cells[1][row_index] = (char *)sqlite3_column_text(stmt, 1);
					 journal_list->Cells[2][row_index] = (char *)sqlite3_column_text(stmt, 2);
					 journal_list->Cells[3][row_index] = (char *)sqlite3_column_text(stmt, 3);
					 journal_list->Cells[4][row_index] = (char *)sqlite3_column_text(stmt, 4);
				} else {
					if (row_index < journal_list->RowCount) {
						journal_list->Rows[row_index]->Clear();
					}
					break;
				}
				journal_list->RowCount = ++row_index;
			}
		}
	} else {
		ShowMessage("��������� ������� ������!");
	}

	sqlite3_free(error_message);
	sqlite3_finalize(stmt);
}
//---------------------------------------------------------------------------
bool __fastcall Tfatca::dlg_user_name()
{
	sqlite3_stmt *stmt;
	char *error_message = 0;
	AnsiString sql_req;

	if (db) {
		//��������� ������ ���� ����� ����� �� �������
		sql_req = "SELECT DISTINCT NAME FROM HISTORY;";
		if (sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0) == SQLITE_OK) {
			user_name_form->uf_name->Items->Clear();
			while (1) {
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					user_name_form->uf_name->Items->Add((char *)sqlite3_column_text(stmt, 0));
				} else {
					break;
				}
			}
		} else {
			ShowMessage(error_message);
		}
		sqlite3_free(error_message);
		sqlite3_finalize(stmt);

		//������������� ��������� ��� � ������� � �������� ��������
		sql_req = "SELECT NAME FROM HISTORY ORDER BY ID DESC LIMIT 1;";
		if (sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0) == SQLITE_OK) {
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				user_name_form->uf_name->Text = (char *)sqlite3_column_text(stmt, 0);
			}
		} else {
			ShowMessage(error_message);
		}
	} else {
		ShowMessage("��������� ������� ������!");
	}
	sqlite3_free(error_message);
	sqlite3_finalize(stmt);

	user_name_form->ShowModal();

	if (user_name.Length()) {
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::fatca_xml_load(bool auto_load)
{
	TStringList * list_path  = new TStringList;
	TStringList * list_space = new TStringList;

	_di_IXMLNodeList cnl = mdl->DocumentElement->ChildNodes->Nodes["List"]->ChildNodes;

	//������� ��� ���������� ���������
	if (fatca_state) { delete [] fatca_state; fatca_state = 0;}

	//�������� ������ ��� �������� ��������� ����� �� ������
	fatca_state = new char[cnl->Count];
	memset(fatca_state, (char)0, cnl->Count);

	//������� �-�� ����� �����. �� ������� ���������� � xml-�����
	//int err_count = 0;

	//���������� fatca xml
	for (int i = 0; i < cnl->Count; i++)
	{
		UnicodeString key  = cnl->Nodes[i]->ChildNodes->FindNode("Title")->GetText();
		AnsiString path = cnl->Nodes[i]->ChildNodes->FindNode("Link")->GetText();

		split(list_path, path, ',');

		_di_IXMLNode txn = xml->DocumentElement->ParentNode;
		for (int j = 0; j < list_path->Count; j++)
		{
			AnsiString node = list_path->Strings[j];
			split(list_space, node, ':');
			UnicodeString nme = list_space->Strings[1];
			UnicodeString spc = xml->DocumentElement->FindNamespaceURI(list_space->Strings[0]);
			list_space->Clear();
			if (txn) {
				txn = txn->ChildNodes->FindNode(nme, spc);
			} else {
				//err_count++;
				break;
			}
		}
		list_path->Clear();

		//���������� ������� ������� �� ���������
		if (txn) {
			_di_IXMLNode attr = cnl->Nodes[i]->ChildNodes->FindNode("Attribute");
			if (attr) {
				fatca_list->Strings->Add(key + "=" + txn->GetAttribute(attr->GetText()));
			} else {
				if (auto_load) {
					fatca_list->Strings->Add(key + "=" + txn->GetText());
				} else if ("sfa:MessageRefId"    == txn->NodeName) {
					fatca_list->Strings->Add(key + "=" + auto_id);
					fatca_state[i] |= FIELD_AUTO_GEN;//���� ������������� �������������
				} else if ("sfa:ReportingPeriod" == txn->NodeName) {
					fatca_list->Strings->Add(key + "=" + UnicodeString(CurrentYear()-1) + "-12-31");
					fatca_state[i] |= FIELD_AUTO_GEN;//���� ������������� �������������
				} else if ("sfa:Timestamp"       == txn->NodeName) {
					fatca_list->Strings->Add(key + "=" + auto_timestamp);
					fatca_state[i] |= FIELD_AUTO_GEN;//���� ������������� �������������
				} else {
					fatca_list->Strings->Add(key + "=" + txn->GetText());
				}
			}
		} else {
			int inserted_row = fatca_list->InsertRow("-"+key, "", 1);
			fatca_list->ItemProps[inserted_row - 1]->ReadOnly = true;
			fatca_state[i] |= FIELD_NOT_FOUND; //�� ������� ����� ����� � xml-�����
			fatca_state[i] |= FIELD_CONSTANT;  //���� ���. ������ ��� ������
		}
	}
	//if (err_count) ShowMessage("������!\n�� ������� ����� " + UnicodeString(err_count) + " ��. ������");
	//����������� ������� ������
	delete list_path;
	delete list_space;
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::meta_xml_load(bool auto_load)
{

	_di_IXMLNodeList mtl = mdl->DocumentElement->ChildNodes->Nodes["Meta"]->ChildNodes;

	//������� ��� ���������� ���������
	if (meta_state) { delete [] meta_state, meta_state = 0;}

	//�������� ������ ��� �������� ��������� ����� �� ������
	meta_state = new char[mtl->Count];
	memset(meta_state, (char)0, mtl->Count);

	//������� �-�� ����� �����. �� ������� ���������� � xml-�����
	//int err_count = 0;

	//���������� ����������
	for (int i = 0; i < mtl->Count; i++) {

		UnicodeString key  = mtl->Nodes[i]->ChildNodes->FindNode("Title")->GetText();
		UnicodeString path = mtl->Nodes[i]->ChildNodes->FindNode("Link")->GetText();
		UnicodeString val = "";

		_di_IXMLNode node_test = mtf->DocumentElement->ChildNodes->FindNode(path);
		if (node_test != 0) {
			val = node_test->GetText();
		} else {
			//err_count++;

			int inserted_row = meta_list->InsertRow("-"+key, "", 1);
			meta_list->ItemProps[inserted_row - 1]->ReadOnly = true;
			meta_state[i] |= FIELD_NOT_FOUND; //�� ������� ����� ����� � xml-�����
			meta_state[i] |= FIELD_CONSTANT;  //���� ���. ������ ��� ������

			continue;
		}
		//����������/�������������� ���� ������� �� xml
		if (auto_load) {
			meta_list->Strings->Add(key + "=" + val);
		} else if (path == "FileCreateTs") {
			int inserted_row = meta_list->InsertRow(key, auto_timestamp + "Z", 1);
			meta_list->ItemProps[inserted_row - 1]->ReadOnly = true;
			meta_state[i] |= FIELD_LINKED;  //���� ������������� ��������
			meta_state[i] |= FIELD_CONSTANT;//������ ��� ������
		} else if (path == "TaxYear") {
			meta_list->Strings->Add(key + "=" + UnicodeString(CurrentYear()-1));
			meta_state[i] |= FIELD_AUTO_GEN;//���� ������������� �������������
		} else {
			meta_list->Strings->Add(key + "=" + val);
		}
	}
	//if (err_count) ShowMessage("������!\n�� ������� ����� " + UnicodeString(err_count) + " ��. ������");
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::menu_aboutClick(TObject *Sender)
{
	UnicodeString version_value, msg;

	version_value = PrintVersionStringInfo(Application->ExeName.c_str());

	msg  = "Peretykin Sergey\n";
	msg += "RUE \"RCSD\"\n";
	msg += "www.centraldepo.by\n";
	msg += "Peretykin@centraldepo.by\n";
	msg += "in 2015 � Minsk";

	if (version_value != "") {
		msg += "\nver. " + version_value;
	}

	ShowMessage(msg);
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::menu_hintClick(TObject *Sender)
{
	fatca_list->ShowHint = menu_hint->Checked;
	_di_IXMLNode hint = cnf->DocumentElement->ChildNodes->FindNode("Config")->ChildNodes->FindNode("Hint");
	if (!hint) {
		cnf->DocumentElement->ChildNodes->FindNode("Config")->AddChild("Hint");
	}
	cnf->DocumentElement->ChildNodes->FindNode("Config")->ChildNodes->FindNode("Hint")->SetText(menu_hint->Checked ? 1 : 0);
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	try {
		if (cnf->XML->Text != config) {
			cnf->SaveToFile("config.txml");
		}
		if (fatca_state) { delete fatca_state, fatca_state = 0;}
		if (meta_state)  { delete meta_state,  meta_state  = 0;}
	} catch (...) {
		ShowMessage("something wrong!");
	}
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::menu_exitClick(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::update_all_xml()
{
	TStringList * list_path  = new TStringList;
	TStringList * list_space = new TStringList;

	//��������� fatca
	_di_IXMLNodeList cnl = mdl->DocumentElement->ChildNodes->Nodes["List"]->ChildNodes;
	for (int i = 0; i < cnl->Count; i++) {
		_di_IXMLNode txn = xml->DocumentElement;
		txn = (txn) ? txn->ParentNode : txn;
		//��������� ����
		_di_IXMLNodeList pnl = cnl->Nodes[i]->ChildNodes;
		for (int k = 0; k < pnl->Count ; k++) {
			if (pnl->Get(k)->NodeName == "Link") {
				AnsiString path = pnl->Get(k)->GetText();
				split(list_path, path, ',');
				for (int j = 0; j < list_path->Count; j++) {
					AnsiString node = list_path->Strings[j];
					split(list_space, node, ':');
					UnicodeString nme = list_space->Strings[1];

					UnicodeString spc = "";
					if (xml->DocumentElement) spc = xml->DocumentElement->FindNamespaceURI(list_space->Strings[0]);
					list_space->Clear();
					if (txn && spc.Length()) {
						txn = txn->ChildNodes->FindNode(nme, spc);
					}
				}
				list_path->Clear();
				UnicodeString inner = fatca_list->Cells[1][i + 1];
				_di_IXMLNode attr = pnl->FindNode("Attribute");

				if (attr && attr->GetText().Length() && txn) {
					txn->SetAttribute(attr->GetText(), fatca_list->Cells[1][i + 1]);
				} else {
					if (txn && inner.IsEmpty()) {
						txn->ChildNodes->Clear();
					} else if (txn) {
						txn->SetText(fatca_list->Cells[1][i + 1]);
					}
					// �������� RefId ��� ���������� � ��
					if (txn && txn->NodeName == "ftc:DocRefId") {
						if (txn->ParentNode && txn->ParentNode->NodeName == "ftc:DocSpec") {
							if (txn->ParentNode->ParentNode && txn->ParentNode->ParentNode->NodeName == "ftc:ReportingFI") {
								auto_nodename = txn->GetText();
							}
						}
					}
					// �������� Timestamp ��� ���������� � ��
					if (txn && txn->NodeName == "sfa:Timestamp") {
						auto_timestamp = txn->GetText();
					}
				}
			}
		}
	}
	//��������� ����������
	_di_IXMLNodeList mtl = mdl->DocumentElement->ChildNodes->Nodes["Meta"]->ChildNodes;
	for (int j = 0; j < mtl->Count; j++) {
		UnicodeString key = mtl->Nodes[j]->ChildNodes->FindNode("Title")->GetText();
		UnicodeString path = mtl->Nodes[j]->ChildNodes->FindNode("Link")->GetText();
		_di_IXMLNode node_test = mtf->DocumentElement->ChildNodes->FindNode(path);
		if (node_test) {
			//��������� ��������� ���� ����� ���������
			if (node_test->NodeName == "FileCreateTs") {
				meta_list->Cells[1][j + 1] = auto_timestamp + "Z";
			}
			node_test->SetText(meta_list->Cells[1][j + 1]);
		}
	}

	//����������� ������� ������
	delete list_path;
	delete list_space;
	//���������� xml � memo
	xml_text->Text = xml->XML->Text;
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::update_xml(TObject *Sender)
{
	update_all_xml();
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::menu_download_xmlClick(TObject *Sender)
{
	if (dlgo->Execute()) {
		if (dlgo->FileName.Length() >= 3) {
			xml->LoadFromFile(dlgo->FileName);
			fatca_list->Strings->Clear(); //�������������� ������� ������
			fatca_xml_load(1);
		} else {
			ShowMessage("�� ����� �������� ��� �����!");
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall Tfatca::menu_download_metaClick(TObject *Sender)
{
	if (dlgo->Execute()) {
		if (dlgo->FileName.Length() >= 3) {
			mtf->LoadFromFile(dlgo->FileName);
			meta_list->Strings->Clear(); //�������������� ������� ������
			meta_xml_load(1);
		} else {
			ShowMessage("�� ����� �������� ��� �����!");
		}
	}
}
//---------------------------------------------------------------------------
int col_fatca=0;
int row_fatca=0;
void __fastcall Tfatca::fatca_listMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	fatca_list->MouseToCell(X, Y, col_fatca, row_fatca);
	_di_IXMLNodeList mtl = mdl->DocumentElement->ChildNodes->Nodes["List"]->ChildNodes;
	if (col_fatca != -1 && row_fatca != -1 && row_fatca != 0 && row_fatca <= mtl->Count) {
		UnicodeString hint = mtl->Nodes[(row_fatca - 1)]->ChildNodes->FindNode("Desc")->GetText();
		if (hint.Length()) {
			fatca_list->Hint = hint;
		} else {
			Application->CancelHint();
			fatca_list->Hint = "";
		}
	} else {
		Application->CancelHint();
		fatca_list->Hint = "";
	}
	Application->ActivateHint(fatca_list->ClientToScreen(TPoint(X,Y)));
}
//---------------------------------------------------------------------------
int col_meta=0;
int row_meta=0;
void __fastcall Tfatca::meta_listMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	meta_list->MouseToCell(X, Y, col_meta, row_meta);
	_di_IXMLNodeList mtl = mdl->DocumentElement->ChildNodes->Nodes["Meta"]->ChildNodes;
	if (col_meta != -1 && row_meta != -1 && row_meta != 0 && row_meta <= mtl->Count) {
		UnicodeString hint = mtl->Nodes[(row_meta - 1)]->ChildNodes->FindNode("Desc")->GetText();
		if (hint.Length()) {
			meta_list->Hint = hint;
		} else {
			Application->CancelHint();
			meta_list->Hint = "";
		}
	} else {
		Application->CancelHint();
		meta_list->Hint = "";
	}
	Application->ActivateHint(meta_list->ClientToScreen(TPoint(X,Y)));
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::menu_uploade_metaClick(TObject *Sender)
{
	update_all_xml();
	if (dlgs->Execute()) {
		if (dlgs->FileName.Length() >= 3) {
			if (dlg_user_name()) {
				history_meta_save();
				history_load();
				mtf->SaveToFile(dlgs->FileName);
			}
		} else {
			ShowMessage("�� ����� �������� ��� �����!");
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::menu_uploade_xmlClick(TObject *Sender)
{
	update_all_xml();
	if (dlgs->Execute()) {
		if (dlgs->FileName.Length() >= 3) {
			if (dlg_user_name()) {
				history_xml_save();
				history_load();
				xml_text->Lines->SaveToFile(dlgs->FileName, TEncoding::UTF8);
			}
		} else {
			ShowMessage("�� ����� �������� ��� �����!");
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::fatca_listDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State)
{
	TColor brush_color, font_color;
	if (!State.Contains(gdFixed) && fatca_state && ACol == 0 && ARow) {

		if (fatca_state[ARow - 1] & FIELD_NOT_FOUND) {
			brush_color = clBtnFace;
			font_color  = clHighlight;
		} else if (fatca_state[ARow - 1] & FIELD_CONSTANT) {
			brush_color = TColor(0x00DAE7DA);//clMoneyGreen;
			font_color  = clHighlight;
		} else if (fatca_state[ARow - 1] & FIELD_AUTO_GEN) {
			brush_color = TColor(0x00CEFFFF);//clInfoBk;
			font_color  = clHighlight;
		} else {
			brush_color = clWindow;
			font_color  = clWindowText;
		}

		fatca_list->Canvas->Brush->Color = brush_color;
		fatca_list->Canvas->Font->Color  = font_color;
		fatca_list->Canvas->TextRect(Rect, Rect.Left+2, Rect.Top+2, fatca_list->Cells[ACol][ARow]);
	}
}
//---------------------------------------------------------------------------

void __fastcall Tfatca::meta_listDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State)
{
	TColor brush_color, font_color;
	if (!State.Contains(gdFixed) && meta_state && ACol == 0 && ARow) {

		if (meta_state[ARow - 1] & FIELD_NOT_FOUND) {
			brush_color = clBtnFace;
			font_color  = clHighlight;
		} else if (meta_state[ARow - 1] & FIELD_CONSTANT) {
			brush_color = TColor(0x00DAE7DA);//clMoneyGreen;
			font_color  = clHighlight;
		} else if (meta_state[ARow - 1] & FIELD_AUTO_GEN) {
			brush_color = TColor(0x00CEFFFF);//clInfoBk;
			font_color  = clHighlight;
		} else {
			brush_color = clWindow;
			font_color  = clWindowText;
		}

		meta_list->Canvas->Brush->Color = brush_color;
		meta_list->Canvas->Font->Color  = font_color;
		meta_list->Canvas->TextRect(Rect, Rect.Left+2, Rect.Top+2, meta_list->Cells[ACol][ARow]);
	}
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::popupPopup(TObject *Sender)
{
	bool test_row = 0;
	for (int i = 1; i < journal_list->RowCount; i++) {
		if (!journal_list->Cells[i][journal_list->Selection.Top].IsEmpty()) {
			test_row = 1;
			break;
		}
	}

	if (test_row) {
		popup_delete_row->Enabled   = true;
		popup_download_xml->Enabled = true;
		popup_upload_xml->Enabled   = true;
	} else {
		popup_delete_row->Enabled   = false;
		popup_download_xml->Enabled = false;
		popup_upload_xml->Enabled   = false;
	}
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::popup_download_xmlClick(TObject *Sender)
{
	AnsiString field_id = journal_list->Cells[0][journal_list->Selection.Top];
	char * bufer = 0;

	//�������� xml �� ��
	bufer = get_xml_blob(field_id);

	if (bufer) {
		if (journal_list->Cells[2][journal_list->Selection.Top] == "DATA") {
			fatca_list->Strings->Clear();//�������������� ������� ������
			xml->LoadFromXML(AnsiString(bufer));
			fatca_xml_load(1);
		} else {
			meta_list->Strings->Clear();//�������������� ������� ������
			mtf->LoadFromXML(AnsiString(bufer));
			meta_xml_load(1);
		}
	} else {
		ShowMessage("�� ������� ������������\n������ �� ������!");
	}

	if (bufer) delete [] bufer;
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::popup_upload_xmlClick(TObject *Sender)
{
	AnsiString field_id = journal_list->Cells[0][journal_list->Selection.Top];
	TStringList * temp_xml = new TStringList();

	//�������� xml �� ��
	char * bufer = get_xml_blob(field_id);
	temp_xml->Text = AnsiString(bufer);

	if (dlgs->Execute()) {
		if (dlgs->FileName.Length() >= 3) {
			temp_xml->SaveToFile(dlgs->FileName);
		} else {
			ShowMessage("�� ����� �������� ��� �����!");
		}
	}

	if (bufer) delete [] bufer;
	delete temp_xml;
}
//---------------------------------------------------------------------------
char* __fastcall Tfatca::get_xml_blob(AnsiString id)
{
	sqlite3_stmt *stmt;
	char *error_message = 0;
	AnsiString sql_req;
	char * bufer = 0;

	//� ����� �������� �������� (������ ������) ��� �������. � ��
	//������� � �������� (��� HIDEN=1) ���� �� ������� � �� �� ID
	if (db) {
		sql_req = "SELECT FILE FROM HISTORY WHERE ID = " + id + ";";
		if (sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0) != SQLITE_OK) {
			ShowMessage(error_message);
		} else {
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				//�������� ������ ���� �������� ����������� ������ ��� ������ � ��������� ���
				bufer = (char *)malloc(sqlite3_column_bytes(stmt, 0));
				strcpy (bufer, (char *)sqlite3_column_blob(stmt, 0));
			} else {
				ShowMessage(error_message);
			}
		}
	} else {
		ShowMessage("��������� ������� ������!");
	}
	sqlite3_free(error_message);
	sqlite3_finalize(stmt);

	return bufer;
}
//---------------------------------------------------------------------------
void __fastcall Tfatca::popup_delete_rowClick(TObject *Sender)
{
	//��������� "���������" ��������������=)
	if (MessageDlg("�� ������������� ������\n������� �� ������� ��� ������?", mtInformation, TMsgDlgButtons() << mbYes << mbNo, 0) != mrYes) {
		return;
	}

	sqlite3_stmt *stmt;
	char *error_message = 0;
	AnsiString sql_req;

	//� ����� �������� �������� (������ ������) ��� �������. � ��
	//������� � �������� (��� HIDEN=1) ���� �� ������� � �� �� ID
	if (db) {
		AnsiString field_id = journal_list->Cells[0][journal_list->Selection.Top];
		sql_req = "UPDATE HISTORY SET HIDE = ? WHERE ID = " + field_id + ";";
		if (sqlite3_prepare(db, sql_req.c_str(), sql_req.Length(), &stmt, 0) != SQLITE_OK) {
			ShowMessage(error_message);
		} else {
			if (sqlite3_bind_int(stmt, 1, 1) == SQLITE_OK) {
				if (sqlite3_step(stmt) != SQLITE_DONE) {
					ShowMessage(error_message);
				}
			} else {
				ShowMessage(error_message);
			}
		}
	} else {
		ShowMessage("��������� ������� ������!");
	}
	sqlite3_free(error_message);
	sqlite3_finalize(stmt);

	//��������� ������� �������
    history_load();
}
//---------------------------------------------------------------------------

