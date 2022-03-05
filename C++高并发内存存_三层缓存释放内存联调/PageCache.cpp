#include"PageCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t NumPage) {//NumPage��ҳ��
	assert(NumPage > 0 && NumPage < NPAGE);

	//����ǰλ��Ͱ���Ƿ���Span
	if (!_SpanList[NumPage].Empty()) {
		return _SpanList->Pop();
	}
	//��Ӧλ�õ�Ͱ�ǿգ�������Ͱ����û��Span,����Span�зֳ�СSpan
	for (size_t i = NumPage + 1; i < NPAGE; i++) {
		if (!_SpanList[i].Empty()) {//��һ��Ͱ���ڣ��зִ�Span��NumPageҳSpan��N-NumPageҳ��Span
			Span* NumPageSpan = new Span;
			Span* NSpan = _SpanList[i].Pop();
			//ͷ��
			NumPageSpan->_PageID = NSpan->_PageID;
			NumPageSpan->_Num = NumPage;
			NSpan->_PageID += NumPage;
			NSpan->_Num -= NumPage;

			//�����µ�Span�ҵ�����Ͱ��
			_SpanList[NSpan->_Num].Insert(_SpanList[NSpan->_Num].begin(), NSpan);
			//����NSpanǰ��ҳ��ӳ���ϵ���������
			IdSpanMap[NSpan->_PageID] = NSpan;
			IdSpanMap[NSpan->_PageID + NSpan->_Num - 1] = NSpan;//�м�ҳû��Ҫ����ӳ���У�ǰ����ҳΪ�˷������

			//����PAGE_ID��Span*ӳ����
			for (PAGE_ID i = 0; i < NumPageSpan->_Num; i++) {//����Numҳ
				IdSpanMap[NumPageSpan->_PageID + i] = NumPageSpan;
			}

			return NumPageSpan;
		}
	}
	//����Ͱ��û��Span,ֱ�����������һ���ռ䣬�����ռ�ҵ����һ��Ͱ��
	Span* BigSpan = new Span;
	void* ptr = SystemAlloc(NPAGE - 1);
	BigSpan->_PageID = (PAGE_ID)ptr >> PAGESIZE;
	BigSpan->_Num = NPAGE - 1;
	_SpanList[BigSpan->_Num].Insert(_SpanList[BigSpan->_Num].begin(), BigSpan);
	//�ڵ����Լ���Central Cache���Ϳռ�
	return NewSpan(NumPage);
}

Span* PageCache::MapObjToSpan(void* obj) {
	//����obj��ҳ��
	PAGE_ID pageId = (PAGE_ID)obj >> PAGESIZE;
	//��ȡ����ڴ����Ǹ�Span
	std::unordered_map<PAGE_ID,Span*>::iterator ret = IdSpanMap.find(pageId);
	if (ret != IdSpanMap.end()) {
		return ret->second;
	}
	else {
		assert(false);//�������Ҳ���
		return nullptr;
	}
}

//��CentralCache��Span����,�ϲ�����ҳ
void PageCache::RetrunPageCache(Span* span) {
	//��Spanǰ��ҳ���кϲ��������ڴ���Ƭ����Ƭ����
	while (true) { //��ǰ�ϲ�
		PAGE_ID prevId = span->_PageID - 1;
		std::unordered_map<PAGE_ID, Span*>::iterator ret = IdSpanMap.find(prevId);
		if (ret == IdSpanMap.end()) {
			break;
		}
		else {
			if (ret->second->IsUse == true) {
				//ǰ������ҳ��Span����ʹ�õ��ڴ治�ϲ�
				break;
			}
			//��ǰSpan�ϲ���С����128KB��ֹͣ�ϲ�,�޷�����
			else if (ret->second->_Num + span->_Num >= NPAGE) {
				break;
			}
			else {
				//�ϲ�
				span->_PageID = ret->second->_PageID;
				span->_Num += ret->second->_Num;
				_SpanList[ret->second->_Num].Erase(ret->second);
				delete ret->second;
			}
		}
	}
	//���ϲ�
	while (true) {
		PAGE_ID nextId = span->_PageID + span->_Num;
		std::unordered_map<PAGE_ID, Span*>::iterator ret = IdSpanMap.find(nextId);
		if (ret == IdSpanMap.end()) {
			break;
		}
		else {
			Span* nextSpan = ret->second;
			if (nextSpan->IsUse == true) {
				break;
			}
			else if (nextSpan->_Num + span->_Num >= NPAGE) {
				break;
			}
			else {
				span->_Num += nextSpan->_Num;
				_SpanList[nextSpan->_Num].Erase(nextSpan);
				IdSpanMap.erase(nextSpan->_Num);
				delete nextSpan;
			}
		}
	}
	//���ϲ���Span�������ӳ��
	_SpanList[span->_Num].Insert(_SpanList[span->_Num].begin(), span);
	span->IsUse = false;
	IdSpanMap[span->_PageID] = span;
	IdSpanMap[span->_PageID + span->_Num - 1] = span;
}