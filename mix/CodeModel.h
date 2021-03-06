/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file CodeModel.h
 * @author Arkadiy Paronyan arkadiy@ethdev.com
 * @date 2014
 * Ethereum IDE client.
 */

#pragma once

#include <memory>
#include <atomic>
#include <map>
#include <QObject>
#include <QThread>
#include <QHash>
#include <libdevcore/Common.h>
#include <libdevcore/Guards.h>

class QTextDocument;

namespace dev
{

namespace solidity
{
	class CompilerStack;
}

namespace mix
{

class CodeModel;
class CodeHighlighter;
class CodeHighlighterSettings;
class QContractDefinition;

//utility class to perform tasks in background thread
class BackgroundWorker: public QObject
{
	Q_OBJECT

public:
	BackgroundWorker(CodeModel* _model): QObject(), m_model(_model) {}

public slots:
	void queueCodeChange(int _jobId);
private:
	CodeModel* m_model;
};

///Compilation result model. Contains all the compiled contract data required by UI
class CompiledContract: public QObject
{
	Q_OBJECT
	Q_PROPERTY(QContractDefinition* contract READ contract)
	Q_PROPERTY(QString contractInterface READ contractInterface CONSTANT)
	Q_PROPERTY(QString codeHex READ codeHex CONSTANT)
	Q_PROPERTY(QString documentId MEMBER m_documentId CONSTANT)

public:
	/// Successful compilation result constructor
	CompiledContract(solidity::CompilerStack const& _compiler, QString const& _contractName, QString const& _source);

	/// @returns contract definition for QML property
	QContractDefinition* contract() const { return m_contract.get(); }
	/// @returns contract definition
	std::shared_ptr<QContractDefinition> sharedContract() const { return m_contract; }
	/// @returns contract bytecode
	dev::bytes const& bytes() const { return m_bytes; }
	/// @returns contract bytecode as hex string
	QString codeHex() const;
	/// @returns contract definition in JSON format
	QString contractInterface() const { return m_contractInterface; }

private:
	uint m_sourceHash;
	std::shared_ptr<QContractDefinition> m_contract;
	QString m_compilerMessage; ///< @todo: use some structure here
	dev::bytes m_bytes;
	QString m_contractInterface;
	QString m_documentId;

	friend class CodeModel;
};


using ContractMap = QHash<QString, CompiledContract*>;

/// Code compilation model. Compiles contracts in background an provides compiled contract data
class CodeModel: public QObject
{
	Q_OBJECT

public:
	CodeModel(QObject* _parent);
	~CodeModel();

	Q_PROPERTY(QVariantMap contracts READ contracts NOTIFY codeChanged)
	Q_PROPERTY(bool compiling READ isCompiling NOTIFY stateChanged)
	Q_PROPERTY(bool hasContract READ hasContract NOTIFY codeChanged)

	/// @returns latest compilation results for contracts
	QVariantMap contracts() const;
	/// @returns compilation status
	bool isCompiling() const { return m_compiling; }
	/// @returns true there is a contract which has at least one function
	bool hasContract() const;
	/// Get contract code by url. Contract is compiled on first access and cached
	dev::bytes const& getStdContractCode(QString const& _contractName, QString const& _url);
	/// Get contract by name
	CompiledContract const& contract(QString _name) const;
	/// Find a contract by document id
	/// @returns CompiledContract object or null if not found
	Q_INVOKABLE CompiledContract* contractByDocumentId(QString _documentId) const;

signals:
	/// Emited on compilation state change
	void stateChanged();
	/// Emitted on compilation complete
	void compilationComplete();
	/// Emitted on compilation error
	void compilationError(QString _error);
	/// Internal signal used to transfer compilation job to background thread
	void scheduleCompilationJob(int _jobId);
	/// Emitted if there are any changes in the code model
	void codeChanged();
	/// Emitted if there are any changes in the contract interface
	void contractInterfaceChanged(QString _documentId);

public slots:
	/// Update code model on source code change
	void registerCodeChange(QString const& _documentId, QString const& _code);
	/// Reset code model for a new project
	void reset(QVariantMap const& _documents);

private:
	void runCompilationJob(int _jobId);
	void stop();
	void releaseContracts();

	std::atomic<bool> m_compiling;
	mutable dev::Mutex x_contractMap;
	ContractMap m_contractMap;
	std::unique_ptr<CodeHighlighterSettings> m_codeHighlighterSettings;
	QThread m_backgroundThread;
	BackgroundWorker m_backgroundWorker;
	int m_backgroundJobId = 0; //protects from starting obsolete compilation job
	std::map<QString, dev::bytes> m_compiledContracts; //by name
	dev::Mutex x_pendingContracts;
	std::map<QString, QString> m_pendingContracts; //name to source
	friend class BackgroundWorker;
};

}

}
