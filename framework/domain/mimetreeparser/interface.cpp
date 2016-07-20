/*
    Copyright (c) 2016 Sandro Knauß <knauss@kolabsystems.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "interface.h"
#include "interface_p.h"

#include "stringhtmlwriter.h"
#include "objecttreesource.h"

#include <KMime/Content>
#include <MimeTreeParser/ObjectTreeParser>
#include <MimeTreeParser/MessagePart>
#include <MimeTreeParser/NodeHelper>

#include <QDebug>

class PartPrivate
{
public:
    PartPrivate(Part *part);
    void appendSubPart(Part::Ptr subpart);

    QVector<Part::Ptr> subParts();

    Part *parent() const;
private:
    Part *q;
    Part *mParent;
    QVector<Part::Ptr> mSubParts;
};

PartPrivate::PartPrivate(Part* part)
    : q(part)
    , mParent(Q_NULLPTR)
{

}

void PartPrivate::appendSubPart(Part::Ptr subpart)
{
    subpart->d->mParent = q;
    mSubParts.append(subpart);
}

Part *PartPrivate::parent() const
{
    return mParent;
}

QVector< Part::Ptr > PartPrivate::subParts()
{
    return mSubParts;
}

Part::Part()
    : d(std::unique_ptr<PartPrivate>(new PartPrivate(this)))
{

}

bool Part::hasSubParts() const
{
    return !subParts().isEmpty();
}

QVector<Part::Ptr> Part::subParts() const
{
    return d->subParts();
}

QByteArray Part::type() const
{
    return "Part";
}

QVector<QByteArray> Part::availableContents() const
{
    return QVector<QByteArray>();
}

QVector<Content::Ptr> Part::content() const
{
    return QVector<Content::Ptr>();
}

QVector<Encryption> Part::encryptions() const
{
    auto parent = d->parent();
    if (parent) {
        return parent->encryptions();
    } else {
        return QVector<Encryption>();
    }
}

QVector<Signature> Part::signatures() const
{
    auto parent = d->parent();
    if (parent) {
        return parent->signatures();
    } else {
        return QVector<Signature>();
    }
}

class ContentPrivate
{
public:
    QByteArray mContent;
    QByteArray mCodec;
    Part *mParent;
    Content *q;
};

Content::Content(const QByteArray& content, Part *parent)
    : d(std::unique_ptr<ContentPrivate>(new ContentPrivate))
{
    d->q = this;
    d->mContent = content;
    d->mCodec = "utf-8";
    d->mParent = parent;
}

Content::~Content()
{
}

QVector< Encryption > Content::encryptions() const
{
    if (d->mParent) {
        return d->mParent->encryptions();
    }
    return QVector<Encryption>();
}

QVector< Signature > Content::signatures() const
{
    if (d->mParent) {
        return d->mParent->signatures();
    }
    return QVector<Signature>();
}

QByteArray Content::content() const
{
    return d->mContent;
}

QByteArray Content::charset() const
{
    return d->mCodec;
}

class AlternativePartPrivate
{
public:
    void fillFrom(MimeTreeParser::AlternativeMessagePart::Ptr part);

    QVector<Content::Ptr> content(const QByteArray &ct) const;

    AlternativePart *q;

    QVector<QByteArray> types() const;

private:
    QMap<QByteArray, QVector<Content::Ptr>> mContent;
    QVector<QByteArray> mTypes;
};

void AlternativePartPrivate::fillFrom(MimeTreeParser::AlternativeMessagePart::Ptr part)
{
    mTypes = QVector<QByteArray>() << "html" << "plaintext";

    auto content = std::make_shared<HtmlContent>(part->htmlContent().toLocal8Bit(), q);
    mContent["html"].append(content);
    content = std::make_shared<PlainTextContent>(part->plaintextContent().toLocal8Bit(), q);
    mContent["plaintext"].append(content);
}

QVector<QByteArray> AlternativePartPrivate::types() const
{
    return mTypes;
}

QVector<Content::Ptr> AlternativePartPrivate::content(const QByteArray& ct) const
{
    return mContent[ct];
}

AlternativePart::AlternativePart()
    : d(std::unique_ptr<AlternativePartPrivate>(new AlternativePartPrivate))
{
    d->q = this;
}

AlternativePart::~AlternativePart()
{

}

QByteArray AlternativePart::type() const
{
    return "AlternativePart";
}

QVector<QByteArray> AlternativePart::availableContents() const
{
    return d->types();
}

QVector<Content::Ptr> AlternativePart::content() const
{
    return d->content(availableContents().first());
}

QVector<Content::Ptr> AlternativePart::content(const QByteArray& ct) const
{
    return d->content(ct);
}

class SinglePartPrivate
{
public:
    void fillFrom(MimeTreeParser::TextMessagePart::Ptr part);
    void fillFrom(MimeTreeParser::HtmlMessagePart::Ptr part);
    void fillFrom(MimeTreeParser::AttachmentMessagePart::Ptr part);
    SinglePart *q;

    QVector<Content::Ptr> mContent;
    QByteArray mType;
};

void SinglePartPrivate::fillFrom(MimeTreeParser::TextMessagePart::Ptr part)
{
    mType = "plaintext";
    mContent.clear();
    foreach (const auto &mp, part->subParts()) {
        mContent.append(std::make_shared<PlainTextContent>(mp->text().toLocal8Bit(), q));
    }
}

void SinglePartPrivate::fillFrom(MimeTreeParser::HtmlMessagePart::Ptr part)
{
    mType = "html";
    mContent.clear();
    mContent.append(std::make_shared<HtmlContent>(part->text().toLocal8Bit(), q));
}

void SinglePartPrivate::fillFrom(MimeTreeParser::AttachmentMessagePart::Ptr part)
{

}

SinglePart::SinglePart()
    : d(std::unique_ptr<SinglePartPrivate>(new SinglePartPrivate))
{
    d->q = this;
}

SinglePart::~SinglePart()
{

}

QVector<QByteArray> SinglePart::availableContents() const
{
    return QVector<QByteArray>() << d->mType;
}

QVector< Content::Ptr > SinglePart::content() const
{
    return d->mContent;
}

QByteArray SinglePart::type() const
{
    return "SinglePart";
}

class MimePartPrivate
{
public:
    void fillFrom(MimeTreeParser::MessagePart::Ptr part);
};

QByteArray MimePart::type() const
{
    return "MimePart";
}

ParserPrivate::ParserPrivate(Parser* parser)
    : q(parser)
    , mNodeHelper(std::make_shared<MimeTreeParser::NodeHelper>())
{

}

void ParserPrivate::setMessage(const QByteArray& mimeMessage)
{
    const auto mailData = KMime::CRLFtoLF(mimeMessage);
    KMime::Message::Ptr msg(new KMime::Message);
    msg->setContent(mailData);
    msg->parse();

    // render the mail
    StringHtmlWriter htmlWriter;
    ObjectTreeSource source(&htmlWriter);
    MimeTreeParser::ObjectTreeParser otp(&source, mNodeHelper.get());

    otp.parseObjectTree(msg.data());
    mPartTree = otp.parsedPart().dynamicCast<MimeTreeParser::MessagePart>();

    mEmbeddedPartMap = htmlWriter.embeddedParts();
    mHtml = htmlWriter.html();

    mTree = std::make_shared<Part>();
    createTree(mPartTree, mTree);
}


void ParserPrivate::createTree(const MimeTreeParser::MessagePart::Ptr &start, const Part::Ptr &tree)
{
    foreach (const auto &mp, start->subParts()) {
        const auto m = mp.dynamicCast<MimeTreeParser::MessagePart>();
        const auto text = mp.dynamicCast<MimeTreeParser::TextMessagePart>();
        const auto alternative = mp.dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        const auto html = mp.dynamicCast<MimeTreeParser::HtmlMessagePart>();
        const auto attachment = mp.dynamicCast<MimeTreeParser::AttachmentMessagePart>();
         if (attachment) {
            auto part = std::make_shared<AttachmentPart>();
            part->d->fillFrom(attachment);
            mTree->d->appendSubPart(part);
         } else if (text) {
            auto part = std::make_shared<SinglePart>();
            part->d->fillFrom(text);
            mTree->d->appendSubPart(part);
        } else if (alternative) {
            auto part = std::make_shared<AlternativePart>();
            part->d->fillFrom(alternative);
            mTree->d->appendSubPart(part);
        } else if (html) {
            auto part = std::make_shared<SinglePart>();
            part->d->fillFrom(html);
            mTree->d->appendSubPart(part);
        } else {
            createTree(m, tree);
        }
    }
}

Parser::Parser(const QByteArray& mimeMessage)
    :d(std::unique_ptr<ParserPrivate>(new ParserPrivate(this)))
{
    d->setMessage(mimeMessage);
}

Parser::~Parser()
{
}

QVector<Part::Ptr> Parser::collectContentParts() const
{
    return collect<Part>(d->mTree, [](const Part::Ptr &p){return p->availableContents().indexOf("html") > -1 || p->availableContents().indexOf("text") > -1;}, [](const Part::Ptr &p){return true;});
}

template <typename T>
QVector<typename T::Ptr> Parser::collect(const Part::Ptr &start, std::function<bool(const Part::Ptr &)> select, std::function<bool(const typename T::Ptr &)> filter) const
{
    QVector<typename T::Ptr> ret;
    foreach (const auto &part, start->subParts()) {
        if (select(part)){
            const auto p = std::dynamic_pointer_cast<T>(part);
            if (p && filter(p)) {
                ret.append(p);
            }
            ret += collect<T>(part, select, filter);
        }
    }
    return ret;
}