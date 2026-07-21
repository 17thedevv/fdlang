import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  Executable
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
  // Đường dẫn tới mellis.exe. 
  // Vì thư mục extension nằm ở `editors/vscode`, ta lùi lại 2 cấp để lấy thư mục `bin` gốc.
  const serverPath = context.asAbsolutePath(path.join('..', '..', 'bin', 'mellis.exe'));

  const run: Executable = {
    command: serverPath,
    args: ['--lsp'],
    options: { cwd: workspace.workspaceFolders?.[0]?.uri.fsPath }
  };

  const serverOptions: ServerOptions = {
    run,
    debug: run
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: 'file', language: 'mellis' }],
  };

  client = new LanguageClient(
    'mellisLsp',
    'Mellis Language Server',
    serverOptions,
    clientOptions
  );

  client.start();
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}
